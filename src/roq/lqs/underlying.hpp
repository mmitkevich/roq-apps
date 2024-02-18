#pragma once
#include "roq/parameter.hpp"
#include "roq/core/types.hpp"
#include "roq/core/market.hpp"
#include "roq/lqs/leg.hpp"


namespace roq::lqs {

struct Pricer;
struct Strategy;

struct Underlying {
  core::Market market;

  core::Double delta;
  core::Double delta_min, delta_max;

  std::vector<core::MarketIdent> legs;

  void remove_leg(core::MarketIdent leg);

  void operator()(const roq::Parameter& p, lqs::Strategy& s);

  void compute(lqs::Strategy& s);
};

} // roq::lqs