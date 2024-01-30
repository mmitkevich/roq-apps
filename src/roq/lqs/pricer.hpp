#pragma once
#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/dispatcher.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/handler.hpp"

#include "roq/lqs/leg.hpp"
#include "roq/lqs/underlying.hpp"

namespace roq::lqs {

static constexpr std::string_view EXCHANGE = "lqs";

struct Pricer : core::Handler {

  Pricer(core::Dispatcher &dispatcher, core::Manager &core);

  void operator()(const roq::Event<roq::ParametersUpdate> & e) override;
  void operator()(const roq::Event<core::Quotes> &e) override;
  void operator()(const roq::Event<core::ExposureUpdate> &) override;

  std::pair<lqs::Underlying&, bool> emplace_underlying(std::string_view symbol, std::string_view exchange);
  std::pair<lqs::Leg&, bool> emplace_leg(std::string_view symbol, std::string_view exchange);
  
  bool get_underlying(lqs::Leg& leg, std::invocable<lqs::Underlying &> auto fn);
  bool get_leg(core::MarketIdent market, std::invocable<lqs::Leg &> auto fn);
  void get_legs(lqs::Underlying& underlying, std::invocable<Leg&> auto fn);
  
  core::Dispatcher &dispatcher;
  core::Manager &core;
private:
  void dispatch(lqs::Leg const& leg);
  void compute(lqs::Leg& leg, lqs::Underlying const& underlying);
private:  
  core::Hash<core::MarketIdent, lqs::Underlying> underlyings;
  core::Hash<core::MarketIdent, lqs::Leg> leg_by_market;
};


inline bool Pricer::get_leg(core::MarketIdent market, std::invocable<lqs::Leg &> auto fn) {
    auto iter = leg_by_market.find(market);
    if(iter == std::end(leg_by_market))
        return false;
    fn(iter->second);
    return true;
}

inline bool Pricer::get_underlying(lqs::Leg& leg, std::invocable<lqs::Underlying &> auto fn) {
    fn(underlyings[leg.underlying]);
    return true;
}

inline void Pricer::get_legs(lqs::Underlying& underlying, std::invocable<Leg&> auto fn) {
  for(auto market: underlying.legs) {
    get_leg(market, fn);
  }
}


}

