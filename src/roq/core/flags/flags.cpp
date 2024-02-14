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
    "config.toml",
    "path/to/config.toml");

ABSL_FLAG(  //
    std::string,
    strategy,
    "strategy",
    "strategy");


ABSL_FLAG(
    int,
    publisher_id,
    0,
    "publisher_id"
);

ABSL_FLAG(
  bool,
  erase_all_orders_on_gateway_not_ready,
  false,
  "erase_all_orders_on_gateway_not_ready"
);

namespace roq {
namespace core {

const std::string& Flags::config_file() {
  static const std::string result = absl::GetFlag(FLAGS_config_file);
  return result;
}

const std::string& Flags::strategy() {
  static const std::string result = absl::GetFlag(FLAGS_strategy);
  return result;
}

int Flags::publisher_id() {
  static const int result = absl::GetFlag(FLAGS_publisher_id);
  return result;
  //return 0;
}

bool Flags::erase_all_orders_on_gateway_not_ready() {
  static const bool result = absl::GetFlag(FLAGS_erase_all_orders_on_gateway_not_ready);
  return result;
}

}  // namespace mmaker
}  // namespace roq
