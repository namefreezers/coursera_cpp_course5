#include "test_runner.h"

template<typename T, typename ... Args>
bool EqualsToOneOf(const T& first, const Args& ... args) {
    return ((first == args) || ...);
}

// EqualsToOneOf(x, "apple", "orange") означает (x == "apple" || x == "orange")

void Test() {
  auto x = "pear";
  ASSERT(EqualsToOneOf(x, "pear"));
  ASSERT(!EqualsToOneOf(x, "apple"));
  ASSERT(EqualsToOneOf(x, "apple", "pear"));
  ASSERT(!EqualsToOneOf(x, "apple", "banana"));
  ASSERT(EqualsToOneOf(x, "apple", "banana", "pear"));
  ASSERT(!EqualsToOneOf(x, "apple", "banana", "peach"));
  ASSERT(EqualsToOneOf(x, "apple", "banana", "pear", "orange"));
  ASSERT(!EqualsToOneOf(x, "apple", "banana", "peach", "orange"));

  ASSERT(!EqualsToOneOf(x));

}

int main() {
  TestRunner tr;
  RUN_TEST(tr, Test);
  return 0;
}
