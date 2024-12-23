#include <catch.hpp>

#include <ptr/shared.h>
#include <ptr/weak.h>

#include <string>
#include <utility>

namespace calls {
  
int constructor = 0;
int destructor = 0;
void reset() {
  constructor = destructor = 0;
}

} // namespace calls

template <typename Object>
struct Wrap : public Object {
  template <typename... Args>
  Wrap(Args&&... args)
  : Object(std::forward<Args>(args)...) {
    ++calls::constructor;
  }

  ~Wrap() {
    ++calls::destructor;
  }
};

TEST_CASE("breathing") {
  {
    ptr::Shared<Wrap<std::string>> s1{new Wrap<std::string>{"hello"}};
    ptr::Shared<Wrap<std::string>> s2 = s1;
    REQUIRE(calls::constructor == 1);
    REQUIRE(calls::destructor == 0);
  }
  REQUIRE(calls::destructor == 1);

  calls::reset();
  ptr::Shared<Wrap<std::string>> s3;
  {
    s3 = ptr::make_shared<Wrap<std::string>>("world!!!");
    ptr::Weak<Wrap<std::string>> w1{s3};
    auto s4 = w1.lock();
    ptr::Weak<const Wrap<std::string>> w2{w1};
  }
  s3.reset();
  REQUIRE(calls::constructor == 1);
  REQUIRE(calls::destructor == 1);
}

