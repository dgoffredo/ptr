#include <ptr/shared.h>
#include <ptr/weak.h>

#include <iostream>
#include <string>
#include <typeinfo>
#include <utility>

template <typename Object>
struct Wrap : public Object {
  template <typename... Args>
  Wrap(Args&&... args)
  : Object(std::forward<Args>(args)...) {
    std::cout << "Creating a " << typeid(Object).name() << '\n';
  }

  ~Wrap() {
   std::cout << "Destroying a " << typeid(Object).name() << '\n';
  }
};

int main() {
  ptr::Shared<Wrap<std::string>> s1{new Wrap<std::string>{"hello"}};
  ptr::Shared<Wrap<std::string>> s2 = s1;
  s1.reset();
  auto s3 = ptr::make_shared<Wrap<std::string>>("world!!!");
  swap(s1, s3);
  
  ptr::Weak<Wrap<std::string>> w1{s3};
  w1 = s1;
  auto s4 = w1.lock();

  ptr::Weak<const Wrap<std::string>> w2{w1};
}

