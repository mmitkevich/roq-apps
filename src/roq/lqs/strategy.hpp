#pragma once
#include "roq/lqs/leg.hpp"
#include "roq/lqs/underlying.hpp"
#include "roq/core/hash.hpp"

namespace roq::lqs {

struct Pricer;

struct Strategy {

  Strategy(lqs::Pricer& pricer) 
  : pricer(pricer) {}

  Strategy(Strategy const&) = default;
  Strategy(Strategy&&) = default;

  bool operator()(roq::Parameter const & p);
  bool operator()(core::Quotes const& u);
  bool operator()(core::Exposure const& e);

  std::pair<lqs::Underlying&, bool> emplace_underlying(core::Market const& key);
  std::pair<lqs::Leg&, bool> emplace_leg(core::Market const& key);
  
  bool get_underlying(lqs::Leg& leg, std::invocable<lqs::Underlying &> auto fn);
  bool get_leg(core::MarketIdent market, std::invocable<lqs::Leg &> auto fn);
  void get_legs(lqs::Underlying& underlying, std::invocable<Leg&> auto fn);
  void get_legs(std::invocable<Leg&> auto fn);
  bool compute(lqs::Leg& this_leg);
public:
  lqs::Pricer& pricer;
  bool enabled = false;
  bool auto_legs = true;
  core::StrategyIdent strategy;
  core::PortfolioIdent portfolio; // NOTE: for now portfolio == strategy always
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
    if(!leg.underlying)
      return false;
    auto iter = underlyings.find(leg.underlying);
    if(iter==std::end(underlyings))
      return false;
    fn(iter->second);
    return true;
}

inline void Strategy::get_legs(lqs::Underlying& underlying, std::invocable<Leg&> auto fn) {
  for(auto market: underlying.legs) {
    get_leg(market, fn);
  }
}

inline void Strategy::get_legs(std::invocable<Leg&> auto fn) {
  for(auto& [market, leg]: leg_by_market) {
    fn(leg);
  }
}

} // roq::lqs