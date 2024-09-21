/*
  C++ Co-routine task concept.

  SPDX-License-Identifier: Unlicense

  https://five-embeddev.com/

*/

#ifndef NOP_TASK_HPP
#define NOP_TASK_HPP

#include <coroutine>
#include <cstdint>
#include <atomic>
#include <array>

#ifdef HOST_EMULATION
#include <memory>
#endif

namespace {
#ifdef HOST_EMULATION
    static constexpr size_t TASK_HEAP_SIZE = 4096;
#else
    static constexpr size_t TASK_HEAP_SIZE = 512;
#endif
}// namespace


#ifdef HOST_EMULATION
#define LOG_TASK_STATE(state) \
    fprintf(stderr, #state " nop_task: %p\n", (void*)this);
#define LOG_PROMISE_METHOD(method) \
    fprintf(stderr, #method ": %p\n", (void*)this);
#else
#define LOG_TASK_STATE(state)
#define LOG_PROMISE_METHOD(method)
#endif


struct nop_task_promise_type;

/** This structure represents a task. It is the return type of the functions will be run asynchronously.
    Functions that return this can call co_await.

 */
struct nop_task {


#ifdef HOST_EMULATION
    nop_task(std::shared_ptr<bool> done)
        : done_(done) {
        LOG_TASK_STATE(new);
    }
#endif
    nop_task() {
        LOG_TASK_STATE(new);
    }
    ~nop_task() {
        LOG_TASK_STATE(destroy);
    }


    using promise_type = nop_task_promise_type;

    // For host testing only.
    [[nodiscard]] bool done(void) const noexcept {
#ifdef HOST_EMULATION
        if (done_) {
            return *done_;
        }
#endif
        return false;
    }

  private:
#ifdef HOST_EMULATION
    std::shared_ptr<bool> done_;
#endif
    friend struct nop_task_promise_type;
};

/** Structure that implements the promise object used by co-routines.
 */
struct nop_task_promise_type {

    nop_task get_return_object() {
#ifdef HOST_EMULATION
        done_ = std::make_shared<bool>(false);
        LOG_PROMISE_METHOD(get_return_object);
        return nop_task{ done_ };
#else
        return {};
#endif
    }
    std::suspend_never initial_suspend() {
        LOG_PROMISE_METHOD(initial_suspend);
        return {};
    }
    std::suspend_never final_suspend() noexcept {
        LOG_PROMISE_METHOD(final_suspend);
        return {};
    }
    void return_void() {
        LOG_PROMISE_METHOD(return_void);
#ifdef HOST_EMULATION
        if (done_) {
            *done_ = true;
        }
#endif
    }
    void unhandled_exception() {
        LOG_PROMISE_METHOD(unhandled_exception);
#ifdef HOST_EMULATION
        if (done_) {
            *done_ = true;
        }
#endif
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
        if ((task_heap_index_ + size) < TASK_HEAP_SIZE) {
            // Simply allocate the next region in the task heap
            auto retval = static_cast<void*>(&task_heap_[task_heap_index_]);
            task_heap_index_ += size;
            return retval;
        }
        return nullptr;
    }
    static void operator delete(void* ptr) {
        // TODO - This implementation only allows for a fixed number of tasks.
        (void)ptr;
    }

    inline static std::array<std::byte, TASK_HEAP_SIZE> task_heap_;
    inline static std::size_t task_heap_index_{ 0 };

  private:
#ifdef HOST_EMULATION
    std::shared_ptr<bool> done_;
#endif
    friend struct nop_task;
};


#endif// NOP_TASK
