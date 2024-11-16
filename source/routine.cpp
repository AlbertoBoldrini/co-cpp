#include <iostream>
#include <co/routine.hpp>


namespace co {

void routine::run() { 
    routine *self = this;
    routine *prev = current_routine.get();

    current_routine = self;
  
    __asm__ volatile(   

        // Save current rsp and rbp to prev fiber
        "movq %%rsp, %[prev_sp]\n"
        "movq %%rbp, %[prev_bp]\n"
        
        // Switch to the new fiber's rsp and rbp
        "movq %[next_sp], %%rsp\n"
        "movq %[next_bp], %%rbp\n"

        : [self]    "+r"(self),
          [prev]    "+r"(prev),
          [prev_sp] "=m"(prev->stack_pointer),
          [prev_bp] "=m"(prev->frame_pointer)
        : [next_sp] "rm"(self->stack_pointer),
          [next_bp] "rm"(self->frame_pointer)
        : "memory", "rbx", "r12", "r13", "r14", "r15"
    );  

    if (prev->status == running)
        prev->status = suspended;

    switch(self->status.exchange(running)) {
        case resumed:
            break;

        case created:
            self->starting_point(self->starting_frame);
            prev->run();

        case cancelled:
            throw co::cancelled();
    }
}

thread_local std::shared_ptr<routine> routine::current_thread = std::make_shared<routine>();
thread_local std::shared_ptr<routine> routine::current_routine = routine::current_thread;

} // namespace Kyoto
