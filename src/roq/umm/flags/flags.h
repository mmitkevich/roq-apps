/* Copyright (c) 2021 Mikhail Mitkevich */

#pragma once

#include <string_view>

namespace roq {
inline namespace shared {

struct Flags {
  static bool quoting();
  static bool simulation();
  static std::string_view config_file();
  static std::string_view log_path();
  static std::string_view model();
};


}  // namespace shared
}  // namespace roq
