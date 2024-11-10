#pragma once

// TODO: Maybe use different inclusion style.
#include "detail/control_block.h"
#include "shared.h"

namespace ptr {

template <typename Object>
class Weak {
  Object *object;
  ControlBlock *control_block;
 public:
   // TODO
};

} // namespace ptr
