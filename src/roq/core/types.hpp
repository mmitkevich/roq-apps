#pragma once
#include <cstdint>
#include <span>
#include "roq/core/fmt.hpp"
#include "roq/core/empty_value.hpp"
#include "roq/core/double.hpp"

namespace roq::core {

using Price = Double;
using Volume = Double;
using MarketIdent = uint32_t;
using PortfolioIdent = uint32_t;
using Integer = int64_t;
using Bool = bool;
using String = std::string;

using OrderIdent = uint64_t;
using OrderVersionIdent = uint32_t;

} // roq::core
