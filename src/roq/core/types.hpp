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

} // roq::core
