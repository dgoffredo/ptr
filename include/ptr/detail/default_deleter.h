#pragma once

namespace ptr {

template <typename Object>
struct DefaultDeleter {
  void operator()(Object *object) {
    delete object;
  }
};

} // namespace ptr
