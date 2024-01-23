#pragma once

#include "roq/core/types.hpp"
#include "roq/core/market.hpp"

namespace roq::lqs {

struct Underlying {
  core::Market market;
  roq::Symbol symbol;
  core::Double delta;
};

} // roq::lqs