#pragma once
#include "roq/parameter.hpp"
#include "roq/core/types.hpp"
#include "roq/core/market.hpp"
#include "roq/lqs/leg.hpp"
#include "roq/string_types.hpp"

namespace roq::lqs {

struct Pricer;
struct Strategy;

struct Underlying {
  core::Market market;

  core::Double delta;
  core::Double delta_min = -INFINITY, delta_max = +INFINITY;

  core::BestQuotes market_quotes;

  std::vector<std::pair<core::MarketIdent, roq::Account> > legs;

  core::Double get_delta_by_volume(const lqs::Leg& leg, lqs::Strategy& s);

  void remove_leg(core::Market const& key);
  void add_leg(core::Market const& key);

  void operator()(const roq::Parameter& p, lqs::Strategy& s, std::string_view label);

  void compute(lqs::Strategy& s);
};

} // roq::lqs