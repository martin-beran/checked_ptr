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
    bool current() const { return _current; }
    static std::shared_ptr<const value_type>
        get_shared(const std::shared_ptr<const checked_object>& ptr)
    {
        if (ptr)
            return {ptr, &ptr->_value};
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
    master_ptr(const master_ptr&) = delete;
    master_ptr(master_ptr&&) = delete;
    master_ptr& operator=(const master_ptr&) = delete;
    master_ptr& operator=(master_ptr&&) = delete;
    void set(pointer ptr) {
        std::lock_guard lck(_mtx);
        if (_ptr)
            _ptr->_current = false;
        _ptr = ptr;
    }
private:
    using const_weak_pointer = std::weak_ptr<const checked_type>;
    const_pointer get() const {
        std::lock_guard lck(_mtx);
        return _ptr;
    }
    pointer _ptr;
    mutable std::mutex _mtx;
    friend class checked_shared_ptr<value_type>;
    friend class checked_weak_ptr<value_type>;
};

template <class T> class checked_shared_ptr {
public:
    using value_type = T;
    using shared_pointer = std::shared_ptr<const value_type>;
    using const_shared_pointer = shared_pointer;
    using raw_pointer = const value_type*;
    using const_raw_pointer = raw_pointer;
    checked_shared_ptr(const master_ptr<value_type>& master):
        _master(master), _ptr(_master.get()) {}
    shared_pointer get_shared() const {
        if (!_ptr || !_ptr->current)
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
    using pointer = std::shared_ptr<const value_type>;
    using const_pointer = pointer;
    checked_weak_ptr(const master_ptr<value_type>& master):
        _master(master), _ptr(_master.get()) {}
    pointer get() {
        auto p = _ptr.lock();
        if (!p || !p->current())
            _ptr = p = _master.get();
        return p ? checked_object<value_type>::get_shared(p) : nullptr;
    }
private:
    using master_pointer = typename master_ptr<value_type>::const_weak_pointer;
    const master_ptr<value_type>& _master;
    master_pointer _ptr;
};

}
