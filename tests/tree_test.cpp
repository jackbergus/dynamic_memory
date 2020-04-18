//
// Created by giacomo on 30/03/2020.
//
#include <dynamic_memory>
#include <gtest/gtest.h>
#include <sstream>

struct tree {
    size_t value;
    weak_pointer<struct tree> left, right;

    tree(size_t key, repository<struct tree>* repo) : value{key}, left{repo->new_null_pointer()}, right{repo->new_null_pointer()} {}

    /*friend std::ostream &operator<<(std::ostream &os, const tree &tree) {
        os << "" << tree.value << " {" <<tree.left.memory_holder_pos<< "," <<tree.right.memory_holder_pos <<"}";
        return os;
    }*/

    std::string toString() {
        std::stringstream ss;
        print(ss, 0, false);
        return ss.str();
    }

    void print(std::ostream &os = std::cout, size_t depth = 0, bool isMinus = false) {
        os << std::string(depth*2, '.') << value << " @" << this << std::endl;
        if (left.get()) left->print(os, depth+1, true);
        if (right.get()) right->print(os, depth+1, false);
    }
};

void writeSequenceDown(repository<struct tree>* test_allocator, weak_pointer<struct tree> t, size_t i, std::vector<size_t> &sequence) {
    if (sequence.size() > i) {
        size_t current = (sequence[i]);
        if (!(t.get())) {
            {
                auto newElement = test_allocator->new_element(current, test_allocator);
                t.setGlobal(newElement);
            }
            writeSequenceDown(test_allocator, t, i + 1, sequence);
        } else {
            size_t currentX = (t)->value;
            if (currentX == current) {
                writeSequenceDown(test_allocator, t, i + 1, sequence);
            } else if (currentX < current) {
                writeSequenceDown(test_allocator, (t.operator->()->right), i, sequence);
            } else {
                writeSequenceDown(test_allocator, (t.operator->()->left), i, sequence);
            }
        }
    } // quit otherwise
}

TEST(TreeTest, test1) {
    repository<struct tree> test_allocator;
    weak_pointer<struct tree> root = test_allocator.new_null_pointer();
    std::vector<size_t > v1{5,3,2,1};
    writeSequenceDown(&test_allocator, root, 0, v1);
    //std::cout << test_allocator << std::endl;
    //std::cout << "Printing " << root.memory_holder_pos << std::endl;
    std::stringstream ss;
    root->print(ss); // This test is passed
    //std::cout << std::endl<<std::endl<<std::endl;
    std::vector<size_t> v2{4,3,2,0};
    writeSequenceDown(&test_allocator,root, 0, v2);
    //std::cout << test_allocator << std::endl;
    //std::cout << "Printing " << root.memory_holder_pos << std::endl;
    root->print(ss);
}