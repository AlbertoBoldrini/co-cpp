#ifndef __co_pool_allocator__
#define __co_pool_allocator__
#include <atomic>

namespace co {

template<typename value_type> 
union pool_slot {
    value_type content;
    pool_slot *next;

    pool_slot() noexcept {}
    ~pool_slot() noexcept {}
};

template<typename value_type, typename slot_type=pool_slot<value_type>>
struct pool_allocator {
    pool_allocator() noexcept
        : head(nullptr) 
    {}

    pool_allocator(pool_allocator &&other) noexcept
        : head(other.head.exchange(nullptr, std::memory_order_relaxed)) 
    {}
    
    ~pool_allocator() {
        while (slot_type *slot = get_reserved_slot())
            delete slot;
    }

    value_type* allocate() {
        if (slot_type *slot = get_reserved_slot())
            return &slot->content;
        return &(new slot_type)->content;
    }

    void deallocate(value_type *ptr) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Winvalid-offsetof"
        slot_type *slot = reinterpret_cast<slot_type *>(uintptr_t(ptr) - offsetof(slot_type, content));
        #pragma GCC diagnostic pop
        add_free_slot(slot);
    }

private:
    void add_free_slot(slot_type *slot) {
        slot->next = head.load(std::memory_order_acquire);
        
        while (!head.compare_exchange_weak(slot->next, slot, 
                                           std::memory_order_release,
                                           std::memory_order_acquire));
    }

    slot_type *get_reserved_slot() {
        slot_type *slot = head.load(std::memory_order_acquire);

        while (slot && !head.compare_exchange_weak(slot, slot->next, 
                                                   std::memory_order_release,
                                                   std::memory_order_acquire));
        return slot;
    }

    std::atomic<slot_type*> head;
};

template<typename derived_type, typename slot_type = pool_slot<derived_type>>
struct global_pool {
    static void* operator new(size_t size) {
        return allocator.allocate();
    }

    static void operator delete(void *ptr) {
        allocator.deallocate(reinterpret_cast<derived_type *>(ptr));
    }

    inline static co::pool_allocator<derived_type, slot_type> allocator;
};

} // namespace co

#endif // __co_pool_allocator__