#pragma once
#include "roq/lqs/leg.hpp"
#include "roq/lqs/underlying.hpp"
#include "roq/core/hash.hpp"

namespace roq::lqs {

struct Pricer;

struct Strategy {

  Strategy(core::StrategyIdent strategy_id, lqs::Pricer& pricer) 
  : strategy(strategy_id)
  , pricer(pricer) {}

  Strategy(Strategy const&) = default;
  Strategy(Strategy&&) = default;

  void operator()(roq::Parameter const & p);

  std::pair<lqs::Underlying&, bool> emplace_underlying(std::string_view symbol, std::string_view exchange);
  std::pair<lqs::Leg&, bool> emplace_leg(std::string_view symbol, std::string_view exchange);
  
  bool get_underlying(lqs::Leg& leg, std::invocable<lqs::Underlying &> auto fn);
  bool get_leg(core::MarketIdent market, std::invocable<lqs::Leg &> auto fn);
  void get_legs(lqs::Underlying& underlying, std::invocable<Leg&> auto fn);

  void compute(lqs::Leg& leg, lqs::Underlying const& underlying);
public:
  lqs::Pricer& pricer;
  bool enabled = false;
  core::StrategyIdent strategy;
  core::PortfolioIdent portfolio;
  core::Hash<core::MarketIdent, lqs::Underlying> underlyings;
  core::Hash<core::MarketIdent, lqs::Leg> leg_by_market;
};



inline bool Strategy::get_leg(core::MarketIdent market, std::invocable<lqs::Leg &> auto fn) {
    auto iter = leg_by_market.find(market);
    if(iter == std::end(leg_by_market))
        return false;
    fn(iter->second);
    return true;
}

inline bool Strategy::get_underlying(lqs::Leg& leg, std::invocable<lqs::Underlying &> auto fn) {
    fn(underlyings[leg.underlying]);
    return true;
}

inline void Strategy::get_legs(lqs::Underlying& underlying, std::invocable<Leg&> auto fn) {
  for(auto market: underlying.legs) {
    get_leg(market, fn);
  }
}

} // roq::lqs