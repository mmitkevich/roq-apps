/* Copyright (c) 2021 Mikhail Mitkevich */

#include "flags.hpp"
#include <absl/flags/flag.h>
#include <cstdint>
#include <string_view>
#include <string>
#include <roq/logging.hpp>

ABSL_FLAG(  //
    std::string,
    config_file,
    "",
    "path/to/config.toml");

ABSL_FLAG(  //
    std::string,
    strategy,
    "",
    "strategy");

/*ABSL_FLAG(  //
    std::string,
    log_path,
    "",
    "log path");*/

ABSL_FLAG(
    int,
    publisher_id,
    0,
    "publisher_id"
);

namespace roq {
namespace mmaker {

/*std::string_view Flags::log_path() {
  static const std::string result = absl::GetFlag(FLAGS_log_path);
  return result;
}*/


std::string_view Flags::config_file() {
  static const std::string result = absl::GetFlag(FLAGS_config_file);
  return result;
}

std::string_view Flags::strategy() {
  static const std::string result = absl::GetFlag(FLAGS_strategy);
  return result;
}

int Flags::publisher_id() {
  static const int result = absl::GetFlag(FLAGS_publisher_id);
  return result;
}

}  // namespace mmaker
}  // namespace roq
