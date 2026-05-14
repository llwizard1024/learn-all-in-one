#pragma once

#include <cstddef>
#include <utility>
#include <stdexcept>

template <typename T>
class Vector {
    T* data_;
    size_t size_;
    size_t capacity_;

    void reserve(size_t capacity) {
        if (capacity <= capacity_) return;

        T* new_data = static_cast<T*>(operator new(capacity * sizeof(T)));

        size_t i = 0;

        try {
            for (; i < size_; ++i) {
                new(&new_data[i]) T(std::move(data_[i]));
            }
        } catch (...) {
            for (size_t j = 0; j < i; ++j) {
                new_data[j].~T();
            }
            operator delete(static_cast<void*>(new_data));
            throw;
        }

        for (size_t j = 0; j < size_; ++j) {
            data_[j].~T();
        }
        if (data_) {
            operator delete(static_cast<void*>(data_));
        }

        data_ = new_data;
        capacity_ = capacity;
    }
public:
    Vector() : data_(nullptr), size_(0), capacity_(0) {}

    Vector(const Vector& other) {
        if (other.size_ == 0) {
            data_ = nullptr;
            size_ = 0;
            capacity_ = 0;
            return;
        }

        data_ = static_cast<T*>(operator new(other.size_ * sizeof(T)));
        capacity_ = other.size_;

        for (size_t i = 0; i < other.size_; ++i) {
            new (&data_[i]) T(other.data_[i]);
            ++size_;
        }
    }

    void swap(Vector& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    Vector& operator=(const Vector& other) {
        if (this == &other) return *this;
        Vector temp(other);
        this->swap(temp);

        return *this; 
    }

    Vector(Vector&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_) 
    {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this == &other) return *this;

        for (size_t i = 0; i < size_; ++i) data_[i].~T();
        if (data_) operator delete(static_cast<void*>(data_));

        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;

        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;

        return *this;
    }


    ~Vector() {
        for (size_t i = 0; i < size_; ++i) {
            data_[i].~T();
        }

        if (data_) {
            operator delete(static_cast<void*>(data_));
        }
    }

    void push_back(const T& value) {
        if (size_ == capacity_) {
            reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        }

        new(&data_[size_]) T(value);
        ++size_;
    }

    void push_back(T&& value) {
        if (size_ == capacity_) {
            reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        }

        new(&data_[size_]) T(std::move(value));
        ++size_;
    }

    void pop_back() {
        if (size_ == 0) {
            return;
        }

        data_[size_ - 1].~T();

        size_--;
    }

    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }

    T& at(size_t index) {
        if (index >= size_) throw std::out_of_range("Index out of range");
        return data_[index];
    }
    const T& at(size_t index) const {
        if (index >= size_) throw std::out_of_range("Index out of range");
        return data_[index];
    }

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }

    bool empty() const { return size_ == 0; }
    T& front() { return data_[0]; }
    T& back() { return data_[size_ - 1]; }
    
    const T& front() const { return data_[0]; }
    const T& back() const { return data_[size_ - 1]; }
};