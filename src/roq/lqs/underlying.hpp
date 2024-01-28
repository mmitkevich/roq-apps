#pragma once

#include "roq/core/types.hpp"
#include "roq/core/market.hpp"

namespace roq::lqs {

struct Pricer;

struct Underlying {
  core::Market market;
  roq::Symbol symbol;
  core::Double delta;
  core::Double delta_min, delta_max;


  void compute(lqs::Pricer& pricer);
};

} // roq::lqs