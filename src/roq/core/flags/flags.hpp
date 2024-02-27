// (c) copyright 2023 Mikhail Mitkevich

#pragma once

#include <string_view>

namespace roq {
namespace core {

struct Flags {
  static const std::string& config_file();
  static const std::string& log_path();
  static const std::string& strategy();
  static int publisher_id();
  static bool erase_all_orders_on_gateway_not_ready();
  static bool use_toml_parameters();
};


}  // namespace mmaker
}  // namespace roq
