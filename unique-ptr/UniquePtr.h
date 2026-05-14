#pragma once

template <typename T>
class UniquePtr {
    T* _ptr;
public:
    UniquePtr() : _ptr(nullptr) {}
    explicit UniquePtr(T* other) { _ptr = other; }

    UniquePtr(const T* other) = delete;
    UniquePtr& operator=(const T* other) = delete;


    UniquePtr(UniquePtr&& other) noexcept : _ptr(other._ptr){
        other._ptr = nullptr;
    }
    
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        delete _ptr;
        _ptr = other._ptr;
        other._ptr = nullptr;
        return *this;
    }

    ~UniquePtr() {
        delete _ptr;
    }

    T* release() {
        T* tmp = _ptr;
        _ptr = nullptr;
        return tmp;
    }

    T* get() const {
        return _ptr;
    }

    void reset(T* ptr = nullptr) {
        T* tmp = _ptr;
        _ptr = ptr;
        delete tmp;
    }

    T& operator*() const {
        return *_ptr;
    }

    T* operator->() const {
        return _ptr;
    }

    explicit operator bool() const noexcept {
        return _ptr != nullptr;
    }
};