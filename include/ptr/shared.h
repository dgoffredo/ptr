#pragma once

// TODO: Maybe use different inclusion style.
#include "detail/control_block.h"
#include "detail/default_deleter.h"

#include <cstddef>
#include <memory>
#include <new>

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

 public:
  Shared();
  Shared(std::nullptr_t);
  explicit Shared(Object*);
  template <typename Deleter>
  Shared(Object*, Deleter&&);
  Shared(const Shared&);
  Shared(Shared&&);
  template <typename Managed>
  Shared(const Shared<Managed>&, Object*);
  template <typename Managed>
  Shared(Shared<Managed>&&, Object*);

  ~Shared();

  Shared& operator=(const Shared&);
  Shared& operator=(Shared&&);

  void reset();
  void reset(Object*);

  Object& operator*() const;
  Object *operator->() const;

  Shared *get() const;
};

template <typename Object, typename... Args>
Shared<Object> make_shared(Args&&... args);

template <typename Object>
void swap(Shared<Object>&, Shared<Object>&);

// --------------
// Implementation
// --------------

template <typename Object>
Shared<Object>::Shared()
: object(nullptr)
, control_block(nullptr) {}

template <typename Object>
Shared<Object>::Shared(std::nullptr_t)
: Shared() {}

template <typename Object>
Shared<Object>::Shared(Object *raw)
: Shared(raw, DefaultDeleter<Object>{}) {}

template <typename Object>
template <typename Deleter>
Shared<Object>::Shared(Object *raw, Deleter&& deleter)
: object(raw) {
  control_block = new DeletingControlBlock<Object, Deleter>{
    RefCounts{.strong = 1, .weak = 0},
    std::forward<Deleter>(deleter),
    raw};
}

template <typename Object>
Shared<Object>::Shared(const Shared<Object>& other)
: Shared(other, other.object) {}

template <typename Object>
Shared<Object>::Shared(Shared<Object>&& other)
: Shared(std::move(other), other.object) {}

template <typename Object>
template <typename Managed>
Shared<Object>::Shared(const Shared<Managed>& other, Object *alias)
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
template <typename Managed>
Shared<Object>::Shared(Shared<Managed>&& other, Object* alias)
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

  // Decrement the strong ref count, and possibly mark the object as destroyed.
  std::uint64_t expected = control_block->ref_counts.load();
  RefCounts desired;
  do {
    desired = RefCounts::from_word(expected);
    --desired.strong;
  } while (!control_block->ref_counts.compare_exchange_weak(expected, desired.as_word()));

  if (desired.strong == 0) {
    // It wasn't destroyed before, but it is now.
    control_block->destroy();
  }
  if (desired.strong == 0 && desired.weak == 0) {
    // There are no more strong or weak refs, so free the control block.
    delete control_block;
  }
}

template <typename Object>
Shared<Object>& Shared<Object>::operator=(const Shared<Object>& other) {
  if (other.control_block == control_block) {
    object = other.object;
    return *this;
  }

  this->~Shared();
  new (this) Shared(other);
  return *this;
}

template <typename Object>
Shared<Object>& Shared<Object>::operator=(Shared<Object>&& other) {
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
void Shared<Object>::reset(Object *raw) {
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
Shared<Object> *Shared<Object>::get() const {
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
