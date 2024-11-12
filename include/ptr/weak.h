#pragma once

#include <ptr/detail/control_block.h>
#include <ptr/shared.h>

namespace ptr {

template <typename Object>
class Weak {
  Object *object;
  ControlBlock *control_block;

  template <typename Other>
  friend class Weak;

 public:
  template <typename Other>
  explicit Weak(const Shared<Other>&);
  template <typename Other>
  explicit Weak(const Weak<Other>&);
  template <typename Other>
  explicit Weak(Weak<Other>&&);
   
  ~Weak();
   
  template <typename Other>
  Weak& operator=(const Weak<Other>&);
  template <typename Other>
  Weak& operator=(Weak<Other>&&);
  template <typename Other>
  Weak& operator=(const Shared<Other>&);
  
  void reset();

  Shared<Object> lock() const;
};

template <typename Object>
void swap(Weak<Object>&, Weak<Object>&);

// --------------
// Implementation
// --------------

template <typename Object>
template <typename Other>
Weak<Object>::Weak(const Shared<Other>& other)
: object(other.object)
, control_block(other.control_block) {
  if (!control_block) {
    return;
  }

  control_block->increment_weak();
}

template <typename Object>
template <typename Other>
Weak<Object>::Weak(const Weak<Other>& other)
: object(other.object)
, control_block(other.control_block) {
  if (!control_block) {
    return;
  }

  control_block->increment_weak();
}

template <typename Object>
template <typename Other>
Weak<Object>::Weak(Weak<Other>&& other)
: object(other.object)
, control_block(other.control_block) {
  other.object = nullptr;
  other.control_block = nullptr;
}

template <typename Object>
Weak<Object>::~Weak() {
  if (!control_block) {
    return;
  }
  control_block->decrement_weak();
}

template <typename Object>
template <typename Other>
Weak<Object>& Weak<Object>::operator=(const Weak<Other>& other) {
  if (control_block != other.control_block) {
    if (control_block) {
      control_block->decrement_weak();
    }
    if (other.control_block) {
      other.control_block->increment_weak();
    }
    control_block = other.control_block;
  }
  object = other.object;

  return *this;
}

template <typename Object>
template <typename Other>
Weak<Object>& Weak<Object>::operator=(Weak<Other>&& other) {
  if (this == &other) {
    return *this;
  }

  if (control_block && control_block != other.control_block) {
    control_block->decrement_weak();
  }

  object = other.object;
  control_block = other.control_block;

  other.object = nullptr;
  other.control_block = nullptr;

  return *this;
}

template <typename Object>
template <typename Other>
Weak<Object>& Weak<Object>::operator=(const Shared<Other>& other) {
  if (control_block != other.control_block) {
    if (control_block) {
      control_block->decrement_weak();
    }
    if (other.control_block) {
      other.control_block->increment_weak();
    }
    control_block = other.control_block;
  }
  object = other.object;
  
  return *this;
}

template <typename Object>
void Weak<Object>::reset() {
  if (control_block) {
    control_block->decrement_weak();
  }
  object = nullptr;
  control_block = nullptr;
}

template <typename Object>
Shared<Object> Weak<Object>::lock() const {
  if (!control_block) {
    return Shared<Object>{};
  }

  // Increment the strong ref count, but only if it's not currently zero.
  std::uint64_t expected = control_block->ref_counts.load();
  RefCounts desired;
  do {
    desired = RefCounts::from_word(expected);
    if (desired.strong) {
      ++desired.strong;
    }
  } while (!control_block->ref_counts.compare_exchange_weak(expected, desired.as_word()));

  if (desired.strong) {
    return Shared<Object>{object, control_block};
  }
  return Shared<Object>{};
}

} // namespace ptr
