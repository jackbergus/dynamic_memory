#include <dynamic_memory>
#include <iostream>
#include <gtest/gtest.h>

TEST(RepositoryBasicTest, test_null_ptr) {
    repository<size_t> repo;
    {
        weak_pointer<size_t> ptr = repo.new_null_pointer();
    }
}

TEST(RepositoryBasicTest, test_alloc_ptr) {
    repository<size_t> repo;
    {
        weak_pointer<size_t> ptr = repo.new_element(23);
        auto x = *ptr.get();
        assert(ptr.getReferenceCounterToVal() == 1);
    }
}