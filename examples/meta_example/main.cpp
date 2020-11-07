#include "example_tables.h"

#include <iostream>

#include <assert.h>


int main() {
  #ifdef aaa
  example1_info e1 {1, "odin"};
  example2_info e2 {.id = 2, .name = "dva", .strings = {"x", "yy", "zzz"},
      .num = 4, .fkey = {0, &e1}};
  // and
  std::cout << e1;
  std::cout << e2;
  #endif
  return 0;
}
