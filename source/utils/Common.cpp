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
#include <map>


static std::map<io_loglvl, std::string> level_names = {
  {no_log, "no logs"},
  {err_logs, "err"},
  {warn_logs, "warn"},
  {info_logs, "info"},
  {debug_logs, "debug"}
};

std::string io_loglvl_str(io_loglvl ll) {
  auto l = level_names.find(ll);
  return (l != level_names.end()) ? l->second : "undefiend io_loglvl";
}

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
bool is_exists(const std::string &path) {
  return std::filesystem::exists(path);
}

std::string dir_by_path(const std::string &path) {
  return std::filesystem::path(path).parent_path();
}
#endif  // CXX17
