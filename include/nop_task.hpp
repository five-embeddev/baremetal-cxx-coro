/*
  SPDX-License-Identifier: Unlicense
*/

#ifndef NOP_TASK_HPP
#define NOP_TASK_HPP

#include <coroutine>
#include <cstdint>
#include <atomic>
#include <array>

/** This structure represents a task. Itis the return type of the functions will be run asynchronously.
    Functions that return this can call co_await.

 */
struct nop_task {

    static constexpr size_t TASK_HEAP_SIZE=128;

    /** Structure that implements the promise object used by co-routines.
     */
    struct promise_type {
        
        nop_task get_return_object() { 
            return {}; 
        }
        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_never final_suspend() noexcept { 
            return {}; 
        }
        void return_void() {
        }
        void unhandled_exception() {
        }

        /** Define this to avoid needing to use exceptions on operator new() failure.
         */
        static nop_task get_return_object_on_allocation_failure() {
            return {};
        }

        /** Replacement memory allocation for the co-routine state.
           
           https://en.cppreference.com/w/cpp/language/coroutines#Heap_allocation

           Allocate within a local array. Allocation is once off, so
           this is only appropraite for a static set of tasks.

         */
        static void* operator new(std::size_t size) noexcept {
            static std::size_t task_heap_index_ = 0;
            if ((task_heap_index_ + size) < TASK_HEAP_SIZE) {
                // Simply allocate the next region in the task heap
                auto retval = static_cast<void*>(&task_heap_[task_heap_index_]);
                task_heap_index_+=size;
                return retval;
            }
            return nullptr;
        }
        static void operator delete(void *ptr) {
            // TODO - This implementation only allows for a fixed number of tasks.
        }

        inline static std::array<std::byte, TASK_HEAP_SIZE> task_heap_;

    };

};

#endif // NOP_TASK
