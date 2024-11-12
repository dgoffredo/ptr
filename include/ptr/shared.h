#pragma once

#include <ptr/detail/control_block.h>

#include <cstddef>
#include <memory>

namespace ptr {

template <typename Object>
class Shared {
  Object *object;
  ControlBlock *control_block;

  template <typename Obj, typename... Args>
  friend Shared<Obj> make_shared(Args&&...);

  template <typename Obj>
  friend void swap(Shared<Obj>&, Shared<Obj>&);

  template <typename Obj>
  friend class Shared;

  template <typename Obj>
  friend class Weak;

  template <typename Target>
  Shared(Target*, ControlBlock*);

 public:
  Shared();
  Shared(std::nullptr_t);
  template <typename Target>
  explicit Shared(Target*);
  template <typename Target, typename Deleter>
  Shared(Target*, Deleter&&);
  template <typename Other>
  Shared(const Shared<Other>&);
  template <typename Other>
  Shared(Shared<Other>&&);
  template <typename Managed, typename Alias>
  Shared(const Shared<Managed>&, Alias*);
  template <typename Managed, typename Alias>
  Shared(Shared<Managed>&&, Alias*);

  ~Shared();

  template <typename Other>
  Shared& operator=(const Shared<Other>&);
  template <typename Other>
  Shared& operator=(Shared<Other>&&);

  void reset();
  template <typename Target>
  void reset(Target*);

  Object& operator*() const;
  Object *operator->() const;

  Object *get() const;
};

template <typename Object, typename... Args>
Shared<Object> make_shared(Args&&... args);

template <typename Object>
void swap(Shared<Object>&, Shared<Object>&);

// --------------
// Implementation
// --------------

// This constructor is private, for use by `ptr::Weak::lock()`.
template <typename Object>
template <typename Target>
Shared<Object>::Shared(Target *target, ControlBlock *control_block)
: object(target)
, control_block(control_block) {}

template <typename Object>
Shared<Object>::Shared()
: object(nullptr)
, control_block(nullptr) {}

template <typename Object>
Shared<Object>::Shared(std::nullptr_t)
: Shared() {}

template <typename Object>
template <typename Target>
Shared<Object>::Shared(Target *raw)
: Shared(raw, [](Object *object) {delete object;}) {}

template <typename Object>
template <typename Target, typename Deleter>
Shared<Object>::Shared(Target *raw, Deleter&& deleter)
: object(raw) {
  // TODO: Is the control block for an Object or for a Target?
  control_block = new DeletingControlBlock<Target, Deleter>{
    RefCounts{.strong = 1, .weak = 0},
    std::forward<Deleter>(deleter),
    raw};
}

template <typename Object>
template <typename Other>
Shared<Object>::Shared(const Shared<Other>& other)
: Shared(other, other.object) {}

template <typename Object>
template <typename Other>
Shared<Object>::Shared(Shared<Other>&& other)
: Shared(std::move(other), other.object) {}

template <typename Object>
template <typename Managed, typename Alias>
Shared<Object>::Shared(const Shared<Managed>& other, Alias *alias)
: object(alias)
, control_block(other.control_block) {
  if (!control_block) {
    return;
  }

  // Increment the strong ref count.
  std::uint64_t expected = control_block->ref_counts.load();
  RefCounts desired;
  do {
    desired = RefCounts::from_word(expected);
    ++desired.strong;
  } while (!control_block->ref_counts.compare_exchange_weak(expected, desired.as_word()));
}

template <typename Object>
template <typename Managed, typename Alias>
Shared<Object>::Shared(Shared<Managed>&& other, Alias* alias)
: object(alias)
, control_block(other.control_block) {
  if (&other == this) {
    return;
  }
  other.control_block = nullptr;
  other.object = nullptr;
}

template <typename Object>
Shared<Object>::~Shared() {
  if (!control_block) {
    return;
  }

  // Decrement the strong ref count, possibly destroy the object, and possibly
  // delete the control block.
  control_block->decrement_strong();
}

template <typename Object>
template <typename Other>
Shared<Object>& Shared<Object>::operator=(const Shared<Other>& other) {
  if (other.control_block == control_block) {
    object = other.object;
    return *this;
  }

  this->~Shared();
  new (this) Shared(other);
  return *this;
}

template <typename Object>
template <typename Other>
Shared<Object>& Shared<Object>::operator=(Shared<Other>&& other) {
  if (&other == this) {
    return *this;
  }

  this->~Shared();
  new (this) Shared(std::move(other));
  return *this;
}

template <typename Object>
void Shared<Object>::reset() {
  *this = nullptr;
}

template <typename Object>
template <typename Other>
void Shared<Object>::reset(Other *raw) {
  *this = raw;
}

template <typename Object>
Object& Shared<Object>::operator*() const {
  return *object;
}

template <typename Object>
Object *Shared<Object>::operator->() const {
  return object;
}

template <typename Object>
Object *Shared<Object>::get() const {
  return object;
}

template <typename Object, typename... Args>
Shared<Object> make_shared(Args&&... args) {
  Shared<Object> result;

  auto control_block = std::make_unique<InPlaceControlBlock<Object>>(
    RefCounts{.strong = 1, .weak = 0});
  auto *object = new (control_block->storage) Object(std::forward<Args>(args)...);
  result.object = object;
  result.control_block = control_block.release();

  return result;
}

template <typename Object>
void swap(Shared<Object>& left, Shared<Object>& right) {
  using std::swap;
  swap(left.object, right.object);
  swap(left.control_block, right.control_block);
}

} // namespace ptr
