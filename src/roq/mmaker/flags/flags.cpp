/* Copyright (c) 2021 Mikhail Mitkevich */

#include "flags.h"

#include <absl/flags/flag.h>
#include <string_view>
#include <string>

ABSL_FLAG(  //
    bool,
    quoting,
    false,
    "quoting should be explicitly enabled");

ABSL_FLAG(  //
    bool,
    simulation,
    false,
    "requires an event-log");

ABSL_FLAG(  //
    std::string,
    config_file,
    "",
    "path/to/config.toml");

ABSL_FLAG(  //
    std::string,
    model,
    "sine_test",
    "model name");

namespace roq {
inline namespace shared {

bool Flags::quoting() {
  static const bool result = absl::GetFlag(FLAGS_quoting);
  return result;
}

bool Flags::simulation() {
  static const bool result = absl::GetFlag(FLAGS_simulation);
  return result;
}

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
