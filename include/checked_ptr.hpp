#pragma once

#include <atomic>
#include <memory>
#include <mutex>

namespace checked_ptr {

template <class T> class master_ptr;
template <class T> class checked_shared_ptr;
template <class T> class checked_weak_ptr;

template <class T> class checked_object {
public:
    using value_type = T;
    checked_object() = default;
    explicit checked_object(const value_type& v): _value(v) {}
    explicit checked_object(value_type&& v): _value(std::move(v)) {}
    template <class ...A> explicit checked_object(A&&... args):
        _value(std::forward<A>(args)...) {}
    checked_object(const checked_object&) = delete;
    checked_object(checked_object&&) = delete;
    checked_object& operator=(const checked_object&) = delete;
    checked_object& operator=(checked_object&&) = delete;
    bool current() const {
        return std::atomic_load_explicit(&_current, std::memory_order_relaxed);
    }
    static std::shared_ptr<const value_type>
        get_shared(const std::shared_ptr<const checked_object>& ptr)
    {
        if (ptr)
            return {ptr, &ptr->_value};
        else
            return nullptr;
    }
    // this overload does not increment the reference counter in C++20
    static std::shared_ptr<const value_type>
        get_shared(std::shared_ptr<const checked_object>&& ptr)
    {
        if (ptr)
            return {std::move(ptr), &ptr->_value};
        else
            return nullptr;
    }
    static const value_type*
        get_raw(const std::shared_ptr<const checked_object>& ptr)
    {
        return ptr ? &ptr->_value : nullptr;
    }
private:
    std::atomic<bool> _current{true};
    const value_type _value;
    friend class master_ptr<value_type>;
};

template <class T> class master_ptr {
public:
    using value_type = T;
    using checked_type = checked_object<value_type>;
    using pointer = std::shared_ptr<checked_type>;
    using const_pointer = std::shared_ptr<const checked_type>;
    master_ptr() = default;
    explicit master_ptr(pointer ptr) { set(std::move(ptr)); }
    master_ptr(const master_ptr&) = delete;
    master_ptr(master_ptr&&) = delete;
    master_ptr& operator=(const master_ptr&) = delete;
    master_ptr& operator=(master_ptr&&) = delete;
    void set(pointer ptr) {
#if USE_MUTEX
        std::lock_guard lck(_mtx);
        if (_ptr)
            std::atomic_store_explicit(&_ptr->_current, false,
                                       std::memory_order_relaxed);
        _ptr = std::move(ptr);
        if (_ptr)
            std::atomic_store_explicit(&_ptr->_current, true,
                                       std::memory_order_relaxed);
#else
        if (ptr)
            std::atomic_store_explicit(&ptr->_current, true,
                                       std::memory_order_relaxed);
        pointer old =
            std::atomic_exchange_explicit(&_ptr, ptr,
                                          std::memory_order_release);
        if (old && ptr != old)
            std::atomic_store_explicit(&old->_current, false,
                                       std::memory_order_relaxed);
#endif
    }
private:
    using const_weak_pointer = std::weak_ptr<const checked_type>;
    const_pointer get() const {
#if USE_MUTEX
        std::lock_guard lck(_mtx);
        return _ptr && _ptr->current() ? _ptr : nullptr;
#else
        return std::atomic_load_explicit(&_ptr, std::memory_order_acquire);
#endif
    }
    pointer _ptr;
#if USE_MUTEX
    mutable std::mutex _mtx;
#endif
    friend class checked_shared_ptr<value_type>;
    friend class checked_weak_ptr<value_type>;
};

//template <class T> explicit master_ptr(std::shared_ptr<checked_object<T>>) ->
//    master_ptr<T>;

template <class T> explicit master_ptr(typename master_ptr<T>::pointer) ->
    master_ptr<T>;

template <class T> class checked_shared_ptr {
public:
    using value_type = T;
    using shared_pointer = std::shared_ptr<const value_type>;
    using const_shared_pointer = shared_pointer;
    using pointer = shared_pointer;
    using const_pointer = pointer;
    using raw_pointer = const value_type*;
    using const_raw_pointer = raw_pointer;
    explicit checked_shared_ptr(const master_ptr<value_type>& master):
        _master(master), _ptr(_master.get()) {}
    inline
        explicit checked_shared_ptr(const checked_weak_ptr<value_type>& weak);
    checked_shared_ptr(const checked_shared_ptr&) = default;
    checked_shared_ptr(checked_shared_ptr&&) = default;
    checked_shared_ptr& operator=(const checked_shared_ptr&) = default;
    checked_shared_ptr& operator=(checked_shared_ptr&&) = default;
    shared_pointer get_shared() {
        if (!_ptr || !_ptr->current())
            _ptr = _master.get();
        return _ptr ? checked_object<value_type>::get_shared(_ptr) : nullptr;
    }
    raw_pointer get_raw() {
        if (!_ptr || !_ptr->current())
            _ptr = _master.get();
        return _ptr ? checked_object<value_type>::get_raw(_ptr) : nullptr;
    }
private:
    using master_pointer = typename master_ptr<value_type>::const_pointer;
    const master_ptr<value_type>& _master;
    master_pointer _ptr;
};

template <class T> class checked_weak_ptr {
public:
    using value_type = T;
    using shared_pointer = std::shared_ptr<const value_type>;
    using const_shared_pointer = shared_pointer;
    using pointer = shared_pointer;
    using const_pointer = pointer;
    explicit checked_weak_ptr(const master_ptr<value_type>& master):
        _master(master), _ptr(_master.get()) {}
    checked_weak_ptr(const checked_weak_ptr&) = default;
    checked_weak_ptr(checked_weak_ptr&&) = default;
    checked_weak_ptr& operator=(const checked_weak_ptr&) = default;
    checked_weak_ptr& operator=(checked_weak_ptr&&) = default;
    shared_pointer get_shared() {
        auto p = _ptr.lock();
        if (!p || !p->current())
            _ptr = p = _master.get();
        return p ?
            checked_object<value_type>::get_shared(std::move(p)) : nullptr;
    }
    checked_shared_ptr<T> lock() const {
        return checked_shared_ptr<T>(_master);
    }
private:
    using master_pointer = typename master_ptr<value_type>::const_weak_pointer;
    const master_ptr<value_type>& _master;
    master_pointer _ptr;
    friend class checked_shared_ptr<value_type>;
};

template <class T> checked_shared_ptr<T>::checked_shared_ptr(
                                    const checked_weak_ptr<value_type>& weak):
    checked_shared_ptr(weak.lock())
{
    if (!_ptr)
        throw std::bad_weak_ptr{};
}

}
