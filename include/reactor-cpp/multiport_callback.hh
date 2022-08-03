/*
 * Copyright (C) 2022 TU Dresden
 * All rights reserved.
 *
 * Authors:
 *   Tassilo Tanneberger
 */

#ifndef REACTOR_CPP_MULTIPORT_CALLBACK_HH
#define REACTOR_CPP_MULTIPORT_CALLBACK_HH

#include <algorithm>
#include <atomic>
#include <numeric>
#include <iostream>
#include <type_traits>
#include <vector>

namespace multiport {

enum Strategy {
    Callback,
    Linear
};

// fancy custom type_trait taken from stackoverflow 
// which checks if given class has the member function deactivate
template <typename T>
class has_deactivate
{
    using one = char; 
    struct two { char x[2]; }; //NOLINT modernize-use-using

    template <typename C> static auto test( decltype(&C::has_deactivate) ) -> one;
    template <typename C> static auto test(...) -> two;    

public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

// struct which gets handed to the ports to they can talk back
// to the portbank
struct LockedPortList {
    std::atomic<std::size_t>* size_ = nullptr;
    std::vector<std::size_t>* active_ports_ = nullptr;
    Strategy* strategy_ = nullptr;
};

template <class T, class A = std::allocator<T>>
class PortBankCallBack { //NOLINT cppcoreguidelines-special-member-functions
private:
  std::vector<T> data_{};
  std::vector<std::size_t> active_ports_{};
  std::atomic<std::size_t> size_ = 0;
  Strategy strategy_ = Strategy::Callback;

public:
  using allocator_type = A;
  using value_type = typename A::value_type;
  using reference = typename A::reference;
  using const_reference = typename A::const_reference;
  using difference_type = typename A::difference_type;
  using size_type = typename A::size_type;

  using iterator = typename std::vector<T>::iterator;
  using const_iterator = typename std::vector<T>::const_iterator;

  PortBankCallBack() noexcept = default;

  ~PortBankCallBack() noexcept {
    // we need to tell all the connected ports that this portbank is freed
    if constexpr (std::is_pointer<T>::value) {
      if constexpr (!has_deactivate<std::remove_pointer_t<T>>::value) {
        for (auto i = 0; i < data_.size(); i++) {
          data_[i]->deactivate();
        }
      }
    } else {
      if constexpr (has_deactivate<T>::value) {
        for (auto i = 0; i < data_.size(); i++) {
            data_[i].deactivate();
        }
      }
    }
  };

  auto operator==(const PortBankCallBack& other) const noexcept -> bool {
    return std::equal(std::begin(data_), std::end(data_), std::begin(other.data_), std::end(other.data_));
  }
  auto operator!=(const PortBankCallBack& other) const noexcept -> bool { return !(*this == other); };
  inline auto operator[](std::size_t index) noexcept -> T& { return data_[index]; }
  inline auto operator[](std::size_t index) const noexcept -> const T& { return data_[index]; }

  inline auto begin() noexcept -> iterator { return data_.begin(); };
  inline auto begin() const noexcept -> const_iterator { return data_.begin(); };
  inline auto cbegin() const noexcept -> const_iterator { return data_.cbegin(); };
  inline auto end() noexcept -> iterator { return data_.end(); };
  inline auto end() const noexcept -> const_iterator { return data_.end(); };
  inline auto cend() const noexcept -> const_iterator { return data_.cend(); };

  inline void swap(PortBankCallBack& other) { std::swap(data_, other.data_); };
  inline auto size() const noexcept -> size_type { return data_.size(); };
  inline auto max_size() const noexcept -> size_type { return data_.size(); };
  [[nodiscard]] inline auto empty() const noexcept -> bool { return data_.empty(); };

  [[nodiscard]] inline auto get_active_ports() noexcept -> LockedPortList {
    return LockedPortList {
        &size_,
        &active_ports_,
        &strategy_
    };
  }

  inline void reserve(std::size_t size) noexcept {
    data_.reserve(size);
    active_ports_.reserve(2 * size);
  }

  inline void push_back(const T& elem) noexcept {
    data_.push_back(elem);
  }

  template <class... Args>
  inline void emplace_back(Args&&... args) noexcept {
    data_.emplace_back(args...);
  }

  template <class... Args>
  inline void set(std::size_t index, Args&&... args) noexcept {
    data_[index].set(args...);
  }

  [[nodiscard]] inline auto active_ports_indices() const noexcept -> std::vector<std::size_t> { 
    if (strategy_ == Strategy::Linear) {
      std::vector<std::size_t> v(data_.size());
      std::iota (std::begin(v), std::end(v), 0);

      return v;
    }

    return std::vector<std::size_t>(std::begin(active_ports_), std::begin(active_ports_) + size_.load());
  }
};
} // namespace multiport

#endif // REACTOR_CPP_MULTIPORT_CALLBACK_HH


