#pragma once

#include <atomic>
#include <bit>
#include <cstdint>
#include <utility>

namespace ptr {

struct RefCounts {
  std::uint32_t strong;
  std::uint32_t weak;

  static RefCounts from_word(std::uint64_t word) {
    return std::bit_cast<RefCounts>(word);
  }

  std::uint64_t as_word() const {
    return std::bit_cast<std::uint64_t>(*this);
  }
};

struct ControlBlock {
  // `ref_counts` is a `RefCounts::as_word()`.
  std::atomic<std::uint64_t> ref_counts;

  explicit ControlBlock(RefCounts counts)
  : ref_counts(counts.as_word()) {}

  virtual ~ControlBlock() {}

  // Note that `decrement_strong` might `delete this`.
  virtual void decrement_strong() = 0;

  // Note that `decrement_weak` might `delete this`.
  void decrement_weak() {
    std::uint64_t expected = ref_counts.load();
    RefCounts desired;
    do {
      desired = RefCounts::from_word(expected);
      --desired.weak;
    } while (!ref_counts.compare_exchange_weak(expected, desired.as_word()));

    if (desired.weak == 0 && desired.strong == 0) {
      delete this;
    }
  }
};

template <typename Object>
struct InPlaceControlBlock : public ControlBlock {
  alignas(Object) char storage[sizeof(Object)];

  explicit InPlaceControlBlock(RefCounts counts)
  : ControlBlock(counts) {}

  // Note that `decrement_strong` might `delete this`.
  void decrement_strong() override {
    // We must avoid a shared pointer and a weak pointer trying to destroy the
    // object and free the control block, respectively, at the same time.
    // The object's storage is part of the control block.
    // To avoid destroying an object whose storage is being freed, increment
    // the weak ref count temporarily when decrementing the strong ref count.
    std::uint64_t expected = ref_counts.load();
    RefCounts desired;
    do {
      desired = RefCounts::from_word(expected);
      --desired.strong;
      ++desired.weak;
    } while (!ref_counts.compare_exchange_weak(expected, desired.as_word()));

    if (desired.strong == 0) {
      destroy_object();
    }

    decrement_weak();
  }

  Object *object() {
    return std::launder(reinterpret_cast<Object*>(storage));
  }

  void destroy_object() {
    object()->~Object();
  }
};

template <typename Object, typename Deleter>
struct DeletingControlBlock : public ControlBlock, public Deleter {
  // Most `ptr::Shared` have a pointer to the managed `Object`, but
  // `ptr::Shared` instances created using the aliasing constructor have a
  // pointer to some other (sub-)object.
  // To make sure that the _managed_ object is the one that is destroyed in the
  // end, the control block must contain a pointer to the managed object, even
  // though it's only needed when aliasing `ptr::Shared` are involved.
  Object *object;

  template <typename DeleterParam>
  DeletingControlBlock(RefCounts counts, DeleterParam&& deleter, Object *object)
  : ControlBlock(counts)
  , Deleter(std::forward<DeleterParam>(deleter))
  , object(object) {}

  void decrement_strong() override {
    std::uint64_t expected = ref_counts.load();
    RefCounts desired;
    do {
      desired = RefCounts::from_word(expected);
      --desired.strong;
    } while (!ref_counts.compare_exchange_weak(expected, desired.as_word()));

    if (desired.strong == 0) {
      destroy_object();
    }
  }

  void destroy_object() {
    (*this)(object);
  }
};

} // namespace ptr
