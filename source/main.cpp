#include <iostream>

#include <memory>
#include <co/pool_allocator.hpp>
#include <co/shared_object.hpp>
#include <co/routine.hpp>


int test() {
    std::cout << "1: test()" << std::endl;
    co::routine::current_thread->run();
    std::cout << "2: test()" << std::endl;
    
    return 5;
}


int main() {

    co::future<int> routine = co::routine::spawn<test>();
    std::cout << "2: main()" << std::endl;

    try {
        std::cout << "routine.wait() = " << routine.wait() << std::endl;  
    } catch (const std::exception &e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
    

    return 0;
}