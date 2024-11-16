#ifndef __co__shared_object__
#define __co__shared_object__
#include <atomic>
#include <memory>
#include <concepts>

namespace co {

    template<typename derived_type>
    struct shared_object {
        shared_object() noexcept
            : shared_reference_count(0)
        {}

    public:
        std::atomic<int> shared_reference_count;   
    };

    template <typename T>
    concept with_shared_reference = requires(T t) {
        { t.shared_reference_count } -> std::same_as<std::atomic<int>&>;
    };

} // namespace co

namespace std {

    template<co::with_shared_reference type>
    struct shared_ptr<type>  {
        #define ptr _M_ptr

        shared_ptr() noexcept
            : ptr(nullptr)
        {}

        shared_ptr(type *ptr) noexcept
            : ptr(ptr)
        {
            if (ptr)
                ptr->shared_reference_count++;
        }

        shared_ptr(const shared_ptr &other) noexcept
            : ptr(other.ptr)
        {
            if (ptr)
                ptr->shared_reference_count++;
        }

        template<typename other_type>
        shared_ptr(const shared_ptr<other_type> &other) noexcept
            : ptr(other.ptr)
        {
            if (ptr)
                ptr->shared_reference_count++;
        }

        template<typename other_type>
        shared_ptr(shared_ptr<other_type> &&other) noexcept
            : ptr(other.ptr)
        {
            other.ptr = nullptr;
        }

        template<typename other_type>
        shared_ptr& operator=(const shared_ptr<other_type> &other) noexcept {
            if (ptr)
                ptr->shared_reference_count++;
            this->~shared_ptr();
            ptr = other.ptr;
            return *this;
        }

        template<typename other_type>
        shared_ptr& operator=(shared_ptr<other_type> &&other) noexcept {
            this->~shared_ptr();
            ptr = other.ptr;
            other.ptr = nullptr;
            return *this;
        }

        void reset() noexcept {
            this->~shared_ptr();
            ptr = nullptr;
        }

        void swap(shared_ptr &other) noexcept {
            std::swap(ptr, other.ptr);
        }

        ~shared_ptr() noexcept{
            if (ptr && --ptr->shared_reference_count == 0)
                delete ptr;
        }

        type* get() const noexcept {
            return ptr;
        }

        type* operator->() const noexcept {
            return ptr;
        }

        type& operator*() const noexcept {
            return *ptr;
        }

        type& operator[](std::ptrdiff_t idx) const noexcept {
            return ptr[idx];
        }

        explicit operator bool() const noexcept {
            return ptr;
        }
    
    private:
        type *ptr;

        template<typename other_type>
        friend struct shared_ptr;

        #undef ptr
    };

    template<co::with_shared_reference type, typename ...Args> 
    shared_ptr<type> make_shared(Args&& ...args) {
        return new type(std::forward<Args>(args)...);
    }
}

#endif // __co__shared_object__