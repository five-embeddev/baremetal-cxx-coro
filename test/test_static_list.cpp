/*
   Unit tests for custom list with static memory allocation.

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#include <cstdint>

#ifdef HOST_EMULATION
#include <iostream>
#include <thread>
#endif

#include "unity.h"

#include "coro/static_list.hpp"
#include <string>

class test_struct {
  public:
    test_struct(const std::string& name, unsigned int count)
        : name(name)
        , count(count) {
    }
    test_struct(const char* name, unsigned int count)
        : test_struct(std::string(name), count) {
    }
    const std::string name;
    unsigned int count;
};

void test_static_list_insert_remove(void) {
    static constexpr size_t MAX_ELEMS = 32;
    static_list<test_struct, MAX_ELEMS> test_list;
    for (unsigned int i = 0; i < MAX_ELEMS - 2; i++) {
        // Insert 2
        test_list.emplace_back("a", i);
        TEST_ASSERT_EQUAL_UINT(i, test_list.rbegin()->count);
        test_list.emplace_back(test_struct{ "b", i });
        TEST_ASSERT_EQUAL_UINT(i, test_list.rbegin()->count);
        // Remove 1
        test_list.pop_back();
        TEST_ASSERT_EQUAL_UINT(i, test_list.back().count);
        // if (i != 0) {
        //     cmp_i--;
        //     TEST_ASSERT_EQUAL_UINT(i-1, cmp_i->count);
        // }
    }
}
