/* Copyright (c) 2021 Mikhail Mitkevich */

#include "flags.hpp"
#include <absl/flags/flag.h>
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
    model,
    "",
    "model");

/*ABSL_FLAG(  //
    std::string,
    log_path,
    "",
    "log path");*/

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

std::string_view Flags::model() {
  static const std::string result = absl::GetFlag(FLAGS_model);
  return result;
}

}  // namespace mmaker
}  // namespace roq
