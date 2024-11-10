#include <ptr/shared.h>

#include <string>

int main() {
  ptr::Shared<std::string> s1{new std::string{"hello"}};
  ptr::Shared<std::string> s2 = s1;
  auto s3 = ptr::make_shared<std::string>("world");
  s3.reset();
  ptr::Shared<const char> s4{s2, s2->data()};
  (void)s4;
}

