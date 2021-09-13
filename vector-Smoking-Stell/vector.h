#pragma once

#include <cstddef>
#include <iostream>

template <typename T>
class vector {

public:
  using iterator = T*;
  using const_iterator = const T*;

  vector() :
      size_(0),
      capacity_(0),
      data_(nullptr)
  {}

  vector(const vector& other) : vector() {
    change_buffer(other.size_);
    for (size_t i = 0; i != other.size_; i++) {
      new (data_ + size_) T(other[i]);
      size_++;
    }
  }

  vector& operator=(const vector& other) {
    if (&other != this) {
      vector(other).swap(*this);
    }
    return *this;
  }

  ~vector() {
    clear();
    operator delete(data_);
  }

  T& operator[](size_t index) {
    return data_[index];
  }

  const T& operator[](size_t index) const {
    return data_[index];
  }

  T* data() {
    return data_;
  }

  const T* data() const {
    return data_;
  }

  size_t size() {
    return size_;
  }

  T& front() {
    return data_[0];
  }

  const T& front() const {
    return data_[0];
  }

  T& back() {
    return data_[size_ - 1];
  }

  const T& back() const {
    return data_[size_ - 1];
  }

  void push_back(const T& x) {
    if (size_ == capacity_) {
      T tmpx = x;
      change_buffer((capacity_ == 0) ? 1 : (capacity_ * 2));
      new (data_ + size_) T(tmpx);
    } else {
      new (data_ + size_) T(x);
    }
    size_++;
  }

  void pop_back() {
    --size_;
    data_[size_].~T();
  }

  bool empty() const {
    return size_ == 0;
  }

  void reserve(size_t new_cap) {
    if (new_cap > capacity_) {
      change_buffer(new_cap);
    }
  }

  size_t capacity() const {
    return capacity_;
  }

  void shrink_to_fit() {
    if (size_ < capacity_) {
      change_buffer(size_);
    }
  }

  void clear() {
    while (size_ > 0) {
      pop_back();
    }
  }

  void swap(vector& other) noexcept {
    std::swap(other.size_, size_);
    std::swap(other.capacity_, capacity_);
    std::swap(other.data_, data_);
  }

  iterator begin() {
    return data_;
  }

  iterator end() {
    return (data_ + size_);
  }

  const_iterator begin() const {
    return data_;
  }

  const_iterator end() const {
    return (data_ + size_);
  }

  iterator insert(const_iterator pos, T const& x) {
    size_t index = pos - data_;
    push_back(x);
    for (size_t i = size_ - 1; i > index; i--) {
      std::swap(data_[i], data_[i - 1]);
    }
    return data_ + index;
  }

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    size_t begin_of_range = first - data_;
    size_t length_range = last - data_ - begin_of_range;

    for (size_t i = begin_of_range; i < size_ - length_range; i++) {
      std::swap(data_[i], data_[i + length_range]);
    }
    while (length_range > 0) {
      pop_back();
      length_range--;
    }
    return data_ + begin_of_range;
  }

private :
  void static range_delete(T* data_del, size_t range) {
    for (size_t i = range; i > 0; i--) {
      data_del[i - 1].~T();
    }
    operator delete(data_del);
  }

  T* copy_data(const T* data_for_copy, size_t new_cap, size_t size) {
    if (new_cap == 0) {
      return nullptr;
    }

    T* new_data = static_cast <T*>(operator new(new_cap * sizeof(T)));
    for (size_t i = 0; i < size; i++) {
      try {
        new (new_data + i) T(data_for_copy[i]);
      } catch (...) {
        range_delete(new_data, i);
        throw;
      }
    }
    return new_data;
  }

  void change_buffer(size_t new_cap) {
    if (new_cap == 0) {
      operator delete(data_);
      data_ = nullptr;
      capacity_ = 0;
      return;
    }
    T* new_data = copy_data(data_, new_cap, size_);
    range_delete(data_, size_);
    data_ = new_data;
    capacity_ = new_cap;
  }

  size_t size_;
  size_t capacity_;
  T* data_;
};

