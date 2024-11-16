#ifndef __co_impl_routine__
#define __co_impl_routine__
#include <cstdint>
#include <atomic> 
#include <memory>
#include <mutex>

#include <sys/mman.h>
#include "../routine.hpp"

namespace co {



struct routine_slot {
    static constexpr std::size_t page_size = 4096;
    static constexpr std::size_t guard_size = page_size * 1;
    static constexpr std::size_t stack_size = page_size * 32 - sizeof(routine);

    std::uint8_t guard[guard_size] alignas(page_size);
    std::uint8_t stack[stack_size] alignas(page_size);

    union {
        routine content;
        routine_slot *next;
    };

    routine_slot() noexcept {}
    ~routine_slot() noexcept {}

    static void* operator new(std::size_t size) {

        // Map the memory for the stack
        routine_slot *slot = reinterpret_cast<routine_slot*>(
            mmap(
                nullptr, sizeof(routine_slot), 
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
            )
        ); 

        if (slot == MAP_FAILED) 
            throw std::bad_alloc();

        // Protect the first few pages to act as a guard region
        if (mprotect(&slot->guard, sizeof(slot->guard), PROT_NONE)) {
            munmap(slot, sizeof(routine_slot));
            throw std::bad_alloc();
        }
        
        return slot;
    }

    static void operator delete(void *ptr) {
        munmap(ptr, sizeof(routine_slot));
    }

    static routine_slot *to_slot(routine *routine) noexcept {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Winvalid-offsetof"
        return reinterpret_cast<routine_slot*>(reinterpret_cast<std::uint8_t*>(routine) - offsetof(routine_slot, content));
        #pragma GCC diagnostic pop
    }
};

template<typename type>
routine::routine(type &&frame)
    : status(created)
    , stack_pointer(routine_slot::to_slot(this)->stack + routine_slot::stack_size)
    , frame_pointer(routine_slot::to_slot(this)->stack + routine_slot::stack_size)
{
    stack_pointer = reinterpret_cast<type*>(stack_pointer) - 1;
    starting_frame = new (stack_pointer) type(std::forward<type>(frame));
    stack_pointer = (void *)(uintptr_t(stack_pointer) - 512 & ~0xF);
    frame_pointer = stack_pointer;

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpmf-conversions"
    starting_point = reinterpret_cast<void (*)(void*)>(&type::operator());
    #pragma GCC diagnostic pop
}

template<auto func, typename... args_types, typename ret_type>
inline future<ret_type> routine::spawn(args_types &&...args) {
    future<ret_type> fut {*new routine([...args = std::forward<args_types>(args)] mutable {
        try {
            ret_type value = func(std::forward<args_types>(args)...);
            current_routine->value_pointer = &value;
            current_routine->status = succeed;
            (args.~args_types(), ...);
            current_thread->run();
        } catch (...) {
            std::exception_ptr value = std::current_exception();
            current_routine->value_pointer = &value;
            current_routine->status = failed;
            (args.~args_types(), ...);
            current_thread->run();
        }
    })};

    fut->run();
    return fut;        
}

} // namespace co

#endif // __co_routine__