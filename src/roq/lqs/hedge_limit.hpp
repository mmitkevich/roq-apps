#pragma once

#include "roq/lqs/spread.hpp"

namespace roq::lqs {

struct HedgeLimit {
  bool operator()(lqs::Spread& spread, std::invocable<lqs::Leg const&> auto fn);
};


} // roq::lqs