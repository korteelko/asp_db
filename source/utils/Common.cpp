/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "Common.h"

#include <algorithm>
#include <filesystem>

std::string trim_str(const std::string &str) {
  if (str.empty())
    return "";
  auto wsfront = std::find_if_not(str.begin(), str.end(),
      [](int c){return std::isspace(c);});
  return std::string(wsfront, std::find_if_not(str.rbegin(),
      std::string::const_reverse_iterator(wsfront),
      [](int c){return std::isspace(c);}).base());
}

/* todo: add else case */
#ifdef CXX17
bool is_exist(const std::string &path) {
  return std::filesystem::exists(path);
}

std::string dir_by_path(const std::string &path) {
  return std::filesystem::path(path).parent_path();
}
#endif  // CXX17
