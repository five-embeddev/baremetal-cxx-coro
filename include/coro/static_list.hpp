/*
   Task list storage for scheduler for co-routines.

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef STATIC_LIST
#define STATIC_LIST

#include <array>

#ifdef STATIC_LIST_USE_STD_LIST

#include <list>
#include <memory_resource>

/** Allocator for PMR version of std::list
 */
template<typename T, std::size_t N>
class static_list_allocator {
  public:
    static constexpr size_t LIST_ELEM_SIZE = sizeof(uintptr_t) * 2;
    static_list_allocator()
        : pool_{ buffer_.data(), buffer_.size() }
        , allocator_{ &pool_ } {
    }
    std::array<uint8_t, N*(sizeof(T) + LIST_ELEM_SIZE)> buffer_;
    std::pmr::monotonic_buffer_resource pool_;
    std::pmr::polymorphic_allocator<T> allocator_;
};

/** PMR List where all elements are allocated from a static array.
    Mixin of custom allocator and std::list.
*/
template<typename T, std::size_t N>
class static_list : private static_list_allocator<T, N>
    , public std::pmr::list<T> {
  public:
    using iterator = typename std::pmr::list<T>::iterator;
    using const_iterator = typename std::pmr::list<T>::const_iterator;

    static_list()
        : std::pmr::list<T>{ static_list_allocator<T, N>::allocator_ } {}
};

#else// Not #ifdef STATIC_LIST_USE_STD_LIST

template<typename T, std::size_t N>
class static_list;

namespace {

    /** Basic element of a linked list.
        The contained data type is allocated directly in this strucuture.
        The intention is to allocate this type within a flat memory buffer.
    */
    template<typename T>
    struct static_list_node {
        static_list_node* next{ nullptr };
        static_list_node* prev{ nullptr };
        T value;
    };

    /** Iterator for this linked list.
        Can be used to traverse forwards and reverse.
    */
    template<typename T, std::size_t N, bool REVERSE = false>
    struct static_list_iterator {
      public:
        explicit static_list_iterator(static_list_node<T>* v) noexcept
            : v_{ v } {}
        /* Return.
         */
        T* operator->() const noexcept { return &v_->value; }
        T& operator*() const noexcept { return v_->value; }
        /** Move to the next element in the list.
         */
        const static_list_iterator& operator++() noexcept {
            if (v_) {
                if constexpr (REVERSE) {
                    v_ = v_->prev;
                }
                else {
                    v_ = v_->next;
                }
            }
            return *this;
        }
        friend auto operator<=>(const static_list_iterator<T, N, REVERSE>& lhs,
                                const static_list_iterator<T, N, REVERSE>& rhs) = default;

      private:
        static_list_node<T>* v_{ nullptr };
        friend static_list<T, N>;
    };
}// namespace

/** Statically allocated doubly linked list.
    Each element is allocated within a staically allocated buffer.

    A list of allocated and free elements is maintained.

    The interface is consistent with std::list, but not guaranteed to
    be a replacement.

    The list does not use C++ exceptions. Instead errors will result
    in operating system exceptions (SEGV) or hardware exceptions.

 */
template<typename T, std::size_t N>
class static_list {
  public:
    /* Create the linked list.
     * All elements are linked into the free list.
     */
    static_list() noexcept {
        static_list_node<T>* prev{ nullptr };
        static_list_node<T>* this_node{ get_array_entry(0) };
        // Iterate over each element and ensure next and prev are set.
        for (unsigned int i = 0; i < N; i++) {
            this_node->prev = prev;
            if (prev) {
                prev->next = this_node;
            }
            prev = this_node;
            this_node++;
        }
        prev->next = nullptr;
    }

    static_list(static_list&) = delete;
    static_list(static_list&&) = delete;
    static_list& operator=(const static_list&) = delete;
    static_list& operator=(static_list&&) = delete;

    using iterator = const static_list_iterator<T, N, false>;
    using const_iterator = const static_list_iterator<T, N, false>;
    using riterator = const static_list_iterator<T, N, true>;
    using const_riterator = const static_list_iterator<T, N, true>;

    /** Forward iterator to start of list. */
    iterator begin() const noexcept { return iterator(first_); }
    /** Forward iterator to end of list. */
    iterator end() const noexcept { return iterator(nullptr); }
    /** Constant forward iterator to start of list. */
    const_iterator cbegin() const noexcept { return iterator(first_); }
    /** Constant forward iterator to end of list. */
    const_iterator cend() const noexcept { return iterator(nullptr); }
    riterator rbegin() const noexcept { return riterator(last_); }
    riterator rend() const noexcept { return riterator(nullptr); }
    const_riterator crbegin() const noexcept { return const_riterator(last_); }
    /** Constant reverse iterator to end of list. */
    const_riterator crend() const noexcept { return const_riterator(nullptr); }

    /** Return a reference to the first element in the list.
        @note Will cause nullptr dereference error when the list is empty. (SEGV or HW Exception)
     */
    T& front() noexcept {
        return first_->value;
    }

    /** Return a reference to the last element in the list.
        @note Will cause nullptr dereference error when the list is empty. (SEGV or HW Exception)
     */
    T& back() noexcept {
        return last_->value;
    }

    /** Remove the first element in the list,
        return it to the free list.
    */
    void pop_front() noexcept {
        if (first_) {
            auto* next = first_->next;
            if (next) {
                next->prev = nullptr;
            }
            return_free_elem(first_);
            if (first_ == last_) {
                last_ = next;
            }
            first_ = next;
        }
    }

    /** Remove the last element in the list,
        return it to the free list.
    */
    void pop_back() noexcept {
        if (last_) {
            auto* prev = last_->prev;
            if (prev) {
                prev->next = nullptr;
            }
            return_free_elem(last_);
            if (first_ == last_) {
                first_ = prev;
            }
            last_ = prev;
        }
    }

    /** Test for an empty list.
     */
    bool empty() const {
        return first_ == nullptr;
    }

    /** Instanciate an element at before the iterator position in the list.
     */
    template<typename... Args>
    void emplace(iterator i, Args&&... args) {
        auto elem = get_free_elem();
        if (elem) {
            // Allocate in place
            (void)new (reinterpret_cast<unsigned char*>(&elem->value))
                T(std::forward<Args>(args)...);
            // Insert
            if (i == end()) {
                elem->prev = last_;
                elem->next = nullptr;
                if (last_) {
                    last_->next = elem;
                }
                last_ = elem;
                if (!first_) {
                    // Corner case, adding to empty list
                    first_ = elem;
                }
            }
            else {
                auto* i_prev = i.v_->prev;
                auto* next = i.v_;
                elem->prev = i_prev;
                if (i_prev) {
                    i_prev->next = elem;
                }
                else {
                    first_ = elem;
                }
                elem->next = next;
                if (next) {
                    next->prev = elem;
                }
                else {
                    last_ = elem;
                }
            }
        }
    }

    /** Instanciate an element at the last place in the list.
     */
    template<typename... Args>
    void emplace_back(Args&&... args) {
        auto elem = get_free_elem();
        if (elem) {
            // Allocate in place
            (void)new (reinterpret_cast<unsigned char*>(&elem->value))
                T(std::forward<Args>(args)...);
            // Insert
            elem->prev = last_;
            elem->next = nullptr;
            if (last_) {
                last_->next = elem;
            }
            last_ = elem;
            if (!first_) {
                // Corner case, adding to empty list
                first_ = elem;
            }
        }
    }

    /** Erase at iterator position.
     */
    void erase(iterator i) {
        auto org_ptr = i.v_;
        // Remove from list
        auto org_next = org_ptr->next;
        auto org_prev = org_ptr->prev;
        if (org_prev) {
            org_prev->next = org_next;
        }
        else {
            // When prev is not set, this is the first elemnt.
            first_ = org_next;
        }
        if (org_next) {
            org_next->prev = org_prev;
        }
        else {
            // When next is not set, this is the last element.
            last_ = org_prev;
        }
        // Save to free list
        return_free_elem(org_ptr);
    }

  private:
    /** Get the next element in the free list.
        As this list does not allocate memory,
        all elements are stored in a free list.
     */
    static_list_node<T>* get_free_elem() {
        if (free_) {
            auto* elem = free_;
            free_ = elem->next;
            if (free_) {
                free_->prev = nullptr;
            }
            return elem;
        }
        return nullptr;
    }

    /** Return am element to the free list.
        As this list does not allocate memory,
        all elements are stored in a free list.
     */
    void return_free_elem(static_list_node<T>* elem) {
        elem->prev = nullptr;
        elem->next = free_;
        if (free_) {
            free_->prev = elem;
        }
        free_ = elem;
    }

    /** Directly Access the array elements that store the nodes.
     */
    static_list_node<T>* get_array_entry(size_t i) {
        auto v = reinterpret_cast<static_list_node<T>*>(&buffer_[0]);
        return &v[i];
    }

    //! Use and array to store all nodes in the same memory block as this data structure.
    std::array<unsigned char, sizeof(static_list_node<T>) * N> buffer_;
    //! The free elements. nullptr when all elements reserved.
    static_list_node<T>* free_{ get_array_entry(0) };
    //! Start of the list. nullptr when empty.
    static_list_node<T>* first_{ nullptr };
    //! End of the list. nullptr when empty.
    static_list_node<T>* last_{ nullptr };
};


#endif

#endif
