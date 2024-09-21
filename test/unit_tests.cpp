/*
   Unit tests for co-routine scheduling.

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#include "unity.h"

extern void test_static_list_insert_remove();
extern void test_single_coroutine();
extern void test_interleaving_coroutines();
extern void test_nested_coroutines();
extern void test_single_prio_coroutine();
extern void test_single_unordered_coroutine();
extern void test_double_unordered_coroutine();
extern void test_double_unordered_coroutine_blocking_patterns();

void setUp(void) {
    // There is no global state for the co-routine implementation.
}

void tearDown(void) {
  // clean stuff up here
}


int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_static_list_insert_remove);
  RUN_TEST(test_single_coroutine);
  RUN_TEST(test_interleaving_coroutines);
  RUN_TEST(test_nested_coroutines);
  RUN_TEST(test_single_prio_coroutine);
  RUN_TEST(test_single_unordered_coroutine);
  RUN_TEST(test_double_unordered_coroutine);
  RUN_TEST(test_double_unordered_coroutine_blocking_patterns);
  return UNITY_END();
}

// WARNING!!! PLEASE REMOVE UNNECESSARY MAIN IMPLEMENTATIONS //

/**
  * For native dev-platform or for some embedded frameworks
  */
int main(void) {
  return runUnityTests();
}
