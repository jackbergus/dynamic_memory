//
// Created by giacomo on 30/03/2020.
//

#ifndef MULTISET2_REPOSITORY_H
#define MULTISET2_REPOSITORY_H


#include <iostream>
#include <cassert>
#include <vector>
#include <optional>
#include <map>
#include <unordered_set>
#include <set>

#include <dynamicmemory/weak_pointer.h>

template <typename T> class repository {

    std::vector<T> contiguous_memory; ///<@ vector that is actually storing the allocated nodes for the AST
    std::vector<size_t> contiguous_memory_reference_count; ///<@ this contians the reference counters for each strong pointer
    std::vector<std::optional<size_t>> strong_pointers; ///<@ if the strong pointer is not a null pointer, it points to an offset within the contiguous memory
    std::map<size_t, std::unordered_set<size_t>> contiguous_memory_to_multimap; ///<@ getting all the strong pointers pointing to the same value

public:
    ~repository() {
        clear();
    }

    void clear() {
        // By deallocating in this order, I guarantee that all the information is freed in the right order, thus avoiding
        // sigfaults from mutual dependencies within the data structures
        contiguous_memory_to_multimap.clear();
        strong_pointers.clear();
        contiguous_memory_reference_count.clear();
        contiguous_memory.clear();
    }

    template <typename... Ts>
    weak_pointer<T> new_element(Ts&&... args) {
        //assert(newMHP == newPos);
        contiguous_memory.emplace_back(std::forward<Ts>(args)...); // The emplace now might trigger several pointer creations. So, I need to define the newPos differently...
        size_t newPos = contiguous_memory.size()-1;
        size_t newMHP = strong_pointers.size();                      // ... This also applies to the memory_holders, tha tis chained to "contiguous_memory"
        contiguous_memory_reference_count.emplace_back(0);
        strong_pointers.emplace_back(newPos);
        contiguous_memory_to_multimap[newPos].emplace(newMHP);
        return {this, newMHP};
    }

    template <typename... Ts>
    weak_pointer<T>& set_new_element(weak_pointer<T>& ptr, Ts&&... args) {
        //assert(newMHP == newPos);
        contiguous_memory.emplace_back(std::forward<Ts>(args)...); // The emplace now might trigger several pointer creations. So, I need to define the newPos differently...
        size_t newPos = contiguous_memory.size()-1;
        size_t newMHP = strong_pointers.size();                      // ... This also applies to the memory_holders, tha tis chained to "contiguous_memory"
        contiguous_memory_reference_count.emplace_back(0);
        strong_pointers.emplace_back(newPos);
        contiguous_memory_to_multimap[newPos].emplace(newMHP);
        weak_pointer<T> element{this, newMHP};
        ptr.setGlobal(element);
        return ptr;
    }

    /**
     * Creates a null pointer: guarantess that a not all the null pointers shall always point to the same memory region
     * @return
     */
    weak_pointer<T> new_null_pointer() {
        size_t newMHP = strong_pointers.size();
        contiguous_memory_reference_count.emplace_back(0); /// The null pointer still is a pointer that will be allocated. It will have no value assocated to it (no contiguous_memory value is emplaced) but a strong_pointer is created
        strong_pointers.emplace_back(); /// A null pointer is defined by a strong pointer containing no reference to the contiguous memory
        return {this, newMHP};                      /// Pointer to the new strong pointer
    }

    /**
     * Returns whether two strong pointers point to an equivalent value.
     *
     * @param left
     * @param right
     * @return
     */
    bool strongPointerEquality(size_t left, size_t right) {
        const std::optional<size_t>& oleft = strong_pointers[left], &oright = strong_pointers[right];
        return (left == right) ||
                (oleft == oright) ||
                (oleft && oright && contiguous_memory[oleft.value()] == contiguous_memory[oright.value()]);
    }

    [[nodiscard]] std::optional<size_t> resolveToStrongPointer(size_t ptr) const {
        if (strong_pointers.size() <= ptr) {
            return {}; /// Cannot return a pointer that is not there
        } else {
            return strong_pointers.at(ptr);
        }
    }

    T* resolveStrongPointer(const std::optional<size_t>& ref) const {
        if (ref) {
            const size_t& x = ref.value();
            return (contiguous_memory.size() > x) ? (T*)&contiguous_memory.at(x) : nullptr; /// Returning the value if it is actually something good
        } else {
            return nullptr; /// Returning a value only if the pointer is pointing to something in the contiguous memory
        }
    }

    T* resolvePointer(size_t ptr) const {
        if (strong_pointers.size() <= ptr) {
            return nullptr; /// Cannot return a pointer that is not there
        } else {
            return resolveStrongPointer(strong_pointers.at(ptr));
        }
    }

    void increment(size_t ptr) {
        assert(contiguous_memory_reference_count.size() == strong_pointers.size());
        if (ptr < strong_pointers.size()) {
            contiguous_memory_reference_count[ptr]++;
        }
    }

    void decrement(size_t ptr) {
        assert(contiguous_memory_reference_count.size() == strong_pointers.size());
        if (ptr < strong_pointers.size()) {
            contiguous_memory_reference_count[ptr]--;
        }
        if (contiguous_memory_reference_count[ptr] == 0) {
            attempt_dispose_element(ptr);
        }
    }

    size_t getReferenceCounterToVal(size_t strong) {
        auto& x = strong_pointers.at(strong);
        if (x) {
            auto it = contiguous_memory_to_multimap.find(strong);
            assert (it != contiguous_memory_to_multimap.end());
            size_t sum = 0;
            for (size_t k : it->second) {
                sum += contiguous_memory_reference_count[k];
            }
            return sum;
        } else {
            return 0;
        }
    }

    /**
     * All the weak pointers pointing to the same strong pointer to the left, will now point to the same value in the
     * right pointer.
     * @param left
     * @param right
     */
    void setGlobal(size_t left, size_t right) {
        attempt_dispose_element(left);
        strong_pointers[left] = strong_pointers[right]; /// Setting the pointer left to the same value on the right
        auto& x = strong_pointers[right];
        if (x) {
            contiguous_memory_to_multimap[x.value()].emplace(left);
        }
        auto it = toDispose.find(left);
        if (it != toDispose.end()) {
            toDispose.erase(it);
        }
    }

private:

    void dispose_strong_ponter(size_t left) {
        strong_pointers.erase(strong_pointers.begin() + left);
        contiguous_memory_reference_count.erase(contiguous_memory_reference_count.begin() + left);

        std::vector<size_t> keysToDel;
        // Updating all the values in the map
        for (auto it = contiguous_memory_to_multimap.begin(), en = contiguous_memory_to_multimap.end(); it != en; ) {
            std::unordered_set<size_t> values;
            for (const size_t& x : it->second) {
                if (x > left) {
                    values.emplace(x-1);
                } else if (x < left) {
                    values.emplace(x);
                }
            }
            if (values.empty()) {
                keysToDel.emplace_back(it->first);
                //it = contiguous_memory_to_multimap.erase(it);
            } else {
                it->second.swap(values);
            }
            it++;
        }
        for (size_t& x : keysToDel)
            contiguous_memory_to_multimap.erase(contiguous_memory_to_multimap.find(x));

        // Updating all the values
    }

    void dispose_value(size_t pos) {
        assert(contiguous_memory_reference_count[pos] == 0);
        assert(pos < contiguous_memory.size()); // The current element should be there in the contiguous_memory
        contiguous_memory.erase(contiguous_memory.begin() + pos); // Removing the memory allocated in the vector in the current position

        // Removing all the elements from the map, as expected.
        auto posIt = contiguous_memory_to_multimap.find(pos);
        if (posIt != contiguous_memory_to_multimap.end())
            contiguous_memory_to_multimap.erase(posIt);

        // Restructuring the strong pointers: getting all the positions greater than pos
        auto it = contiguous_memory_to_multimap.upper_bound(pos);
        std::unordered_set<size_t> toInsert;
        std::map<size_t, std::unordered_set<size_t>> contiguous_memory_to_multimap2; // Decreased map values
        while (it != contiguous_memory_to_multimap.end()) {
            for (const size_t& strong : it->second) {
                toInsert.emplace(strong); // Getting all the strong pointers pointing at values greater than
            }
            contiguous_memory_to_multimap2[it->first-1] = it->second; // Decreasing the key for all the values
            it = contiguous_memory_to_multimap.erase(it);
        }
        for (size_t k : toInsert) { // Decreasing the stroing pointers value
            auto& x = strong_pointers.at(k);
            assert(x);
            x.value() = x.value() - 1;
        }
        // Copying the updated values
        contiguous_memory_to_multimap.insert(contiguous_memory_to_multimap2.begin(), contiguous_memory_to_multimap2.end());
    }

    std::set<size_t> toDispose;

    void attempt_dispose_element(size_t x) {
        toDispose.emplace(x);
        auto it = toDispose.rbegin();

        // I can start to remove elements only when the maximum
        while ((it != toDispose.rend()) && (*it == (strong_pointers.size()-1))) {
            size_t left = *it;
            bool hasDisposed = false;
            size_t valDisposed = 0;
            const std::optional<size_t>& ref = strong_pointers.at(left); /// Getting which is the actual pointed value, if any
            if (ref) { /// If there is a pointed value;
                auto set_ptr = contiguous_memory_to_multimap.find(ref.value());
                assert(set_ptr != contiguous_memory_to_multimap.end());
                auto it = set_ptr->second.find(left);
                if (set_ptr->second.size() == 1) {
                    assert(it != set_ptr->second.end());
                    hasDisposed = true;
                    valDisposed = ref.value();
                    // Removing the value via dispose_value --->
                }
                if (it != set_ptr->second.end())
                    set_ptr->second.erase(it);
            }
            dispose_strong_ponter(left);
            if (hasDisposed) {
                dispose_value(valDisposed); // <--
            }
            it = decltype(it)(toDispose.erase(std::next(it).base())); // Clear the current element from the set
        }

    }

public:
    /**
     * Printing how the memory and the elements
     * @param os
     * @param repository
     * @return
     */
    friend std::ostream &operator<<(std::ostream &os, const repository &repository) {
        for (size_t i = 0, n = repository.contiguous_memory.size(); i<n; i++) {
            os << '[' << i <<  "] --> |{" << repository.contiguous_memory[i] << "}| == " << repository.contiguous_memory_reference_count[i] << std::endl;
        }
        for (size_t i = 0, n = repository.strong_pointers.size(); i<n; i++) {
            os << '(' << i <<  ") --> ";
            if (repository.strong_pointers[i])
                os << repository.strong_pointers[i].value();
            else
                os << "null";
            os << std::endl;
        }
        return os;
    }

    /// A new class should inherit from repository for a specific type of AST = <typename T> and, for this, I should
    /// implement the lexicographical order soring.
};


#endif //MULTISET2_REPOSITORY_H
