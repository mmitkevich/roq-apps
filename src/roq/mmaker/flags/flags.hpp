/* Copyright (c) 2021 Mikhail Mitkevich */

#pragma once

#include <string_view>

namespace roq {
namespace mmaker {

struct Flags {
  static const std::string& config_file();
  static const std::string& log_path();
  static const std::string& strategy();
  static int publisher_id();
};


}  // namespace mmaker
}  // namespace roq
