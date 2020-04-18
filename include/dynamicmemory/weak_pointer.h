//
// Created by giacomo on 30/03/2020.
//

#ifndef MULTISET2_WEAK_POINTER_H
#define MULTISET2_WEAK_POINTER_H


#include <iostream>

/**
 * The repository is the actual memory allocator, that will contain the references to the strong pointers and to the actual
 * allocated elements
 * @tparam T
 */
template<typename T>
class repository;

/**
 * A weak pointer is just a pointer to a strong pointer, which is held within a repository alongside with the actual
 * allocated data.
 * @tparam T
 */
template<typename T>
class weak_pointer {
    repository<T> *element; // Memory repository that contains the actual information
    size_t strong_ptr_pos;  // Vector position for the current element in the strong pointer holder

public:

    /**
     * Creating a strong pointer by knowing a strong memory pointer position
     * @param element
     * @param strongPtrPos
     */
    weak_pointer(repository<T> *element, size_t strongPtrPos) : element(element), strong_ptr_pos(strongPtrPos) {
        // Increment the reference count in the main repository associated to the strong pointer
        if (element) element->increment(strong_ptr_pos);
    }

    /**
     * Copying a weak pointer that was (possibly) pointing to a new memory allocation
     * @param copy
     */
    weak_pointer(const weak_pointer &copy) : element{copy.element}, strong_ptr_pos{copy.strong_ptr_pos} {
        if (element) element->increment(strong_ptr_pos);
    }


    /**
     * Copying a weak pointer that was (possibly) pointing to a new memory allocation via assignment. This will not
     * change the stroing pointer for all the weak pointers.
     * @param copy
     * @return
     */
    weak_pointer &operator=(const weak_pointer &copy) {
        // Decrement the reference count of the element that was previously pointed
        if (element && (get() != nullptr))
            element->decrement(strong_ptr_pos);
        // Copying the new information
        element = copy.element;
        strong_ptr_pos = copy.strong_ptr_pos;
        // Incrementing the reference count
        if (element) element->increment(strong_ptr_pos);
    }

    /**
     * Demanding the repository to return the pointer if this is not missing
     * @return
     */
    T *get() const {
        // Resolving the pointer as an actual element in the remote reference
        return element ? element->resolvePointer(strong_ptr_pos) : nullptr;
    }

    T *operator->() {
        return get();
    }

    /**
     * Changing the value that is going to be pointed by the strong pointer. This will make all the weak pointers
     * associated to it to point to a new value
     * @param ptr
     */
    void setGlobal(const weak_pointer<T> &ptr) {
        assert(element);
        assert(ptr.element);
        if (element != ptr.element) {
            element = ptr.element;
            std::cerr << "Warning: element!=ptr.element, so I'm using ptr.element" << std::endl;
        }
        element->setGlobal(strong_ptr_pos, ptr.strong_ptr_pos);
    }

    std::optional<size_t> resolveStrongPonter() {
        if (element)
            return element->resolveToStrongPointer(strong_ptr_pos);
        else
            return {};
    }

    ~weak_pointer() {
        // On deallocation, decrement the reference count associated to the strong pointer
        if (element) element->decrement(strong_ptr_pos);
    }

    /**
     * Counting the references to the current element
     * @return
     */
    size_t getReferenceCounterToVal() {
        return element ? element->getReferenceCounterToVal(strong_ptr_pos) : 1;
    }

    bool operator==(const weak_pointer &rhs) const {
        return element == rhs.element && // Two weak pointers are equal if they share the same repository...
                                         (strong_ptr_pos == rhs.strong_ptr_pos || // and if either they point to the same region...
               element->strongPointerEquality(strong_ptr_pos, rhs.strong_ptr_pos)); //... or they point to strong pointers containing equivalent values
    }

    bool operator!=(const weak_pointer &rhs) const {
        return !(rhs == *this);
    }

    // Printing the actual value that is pointed by the strong pointer, if any
    friend std::ostream &operator<<(std::ostream &os, const weak_pointer &pointer) {
        auto ptr = pointer.get();
        if (ptr)
            os << *ptr;
        else
            os << "null";
        return os;
    }

};


#endif //MULTISET2_WEAK_POINTER_H
