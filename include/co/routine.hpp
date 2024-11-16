#ifndef __co_routine__
#define __co_routine__
#include <cstdint>
#include <atomic> 
#include <memory>
#include <mutex>

#include <sys/mman.h>

#include "pool_allocator.hpp"
#include "shared_object.hpp"

namespace co {

struct cancelled {};
struct routine_slot;

template<typename type>
struct future;

struct routine : public shared_object<routine>, public global_pool<routine,routine_slot> {

    enum status_type : std::uint8_t {
        created    = 1 << 0,
        running    = 1 << 1,
        suspended  = 1 << 2,
        cancelled  = 1 << 3,
        succeed    = 1 << 4,
        failed     = 1 << 5,
        resumed    = 1 << 6,
    };

    routine() noexcept
        : status(running) 
    {}
    
    template<auto func, typename... types, typename ret_type=std::invoke_result_t<decltype(func), types...>>
        static future<ret_type> spawn(types &&...args);

    void run() __attribute__((noinline, noclone))
               __attribute__((optimize(
                    "-fno-omit-frame-pointer",
                    "-fno-stack-protector"
               )));

private:
    template<typename type>
        routine(type &&frame);

    std::atomic<status_type> status;

    void *stack_pointer;
    void *frame_pointer;
    void *value_pointer;

    void *starting_frame;
    void (*starting_point) (void *);

    template<typename type>
    friend struct future;

public:
    thread_local static std::shared_ptr<routine> current_thread;
    thread_local static std::shared_ptr<routine> current_routine;  
};

template<typename type>
struct future : public std::shared_ptr<routine> {

    future(routine &r)
        : std::shared_ptr<routine>(&r)
    {}

    type& wait() {
        while (!(get()->status & (routine::succeed | routine::failed)))
            get()->run();

        if (get()->status == routine::succeed)
            return *reinterpret_cast<type*>(get()->value_pointer);
        else
            std::rethrow_exception(*reinterpret_cast<std::exception_ptr*>(get()->value_pointer));
    }
};

} // namespace co

#include "impl/routine.hpp"
#endif // __co_routine__