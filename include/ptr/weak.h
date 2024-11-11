#pragma once

#include <ptr/detail/control_block.h>
#include <ptr/shared.h>

namespace ptr {

template <typename Object>
class Weak {
  Object *object;
  ControlBlock *control_block;
 public:
   // TODO
};

} // namespace ptr
