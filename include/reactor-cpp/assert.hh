/*
 * Copyright (C) 2019 TU Dresden
 * All rights reserved.
 *
 * Authors:
 *   Christian Menard
 */

#pragma once

#ifdef REACTOR_CPP_VALIDATE
constexpr bool runtime_validation = REACTOR_CPP_VALIDATE;
#else
constexpr bool runtime_validation = false;
#endif

#ifdef NDEBUG
constexpr bool runtime_assertion = NDEBUG;
#else
constexpr bool runtime_assertion = false;
#endif

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string>

namespace reactor {

class ValidationError : public std::runtime_error {
 private:
  static std::string build_message(const std::string& msg);

 public:
  explicit ValidationError(const std::string& msg)
      : std::runtime_error(build_message(msg)) {}
};

constexpr inline void validate(bool condition, const std::string& message) {
  if constexpr (runtime_validation && !condition) {
    throw ValidationError(message);
  }
}

constexpr inline void toggle_assert([[maybe_unused]] bool condition) {
  if constexpr (runtime_assertion){
    assert(condition);
  }
}

}  // namespace reactor
