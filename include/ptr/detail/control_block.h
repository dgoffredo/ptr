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

  // Destroy the managed object, and possibly deallocate its storage.
  virtual void destroy() = 0;
};

template <typename Object>
struct InPlaceControlBlock : public ControlBlock {
  alignas(Object) char storage[sizeof(Object)];

  explicit InPlaceControlBlock(RefCounts counts)
  : ControlBlock(counts) {}

  Object *object() {
    return std::launder(reinterpret_cast<Object*>(storage));
  }

  void destroy() override {
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

  void destroy() override {
    (*this)(object);
  }
};

} // namespace ptr
