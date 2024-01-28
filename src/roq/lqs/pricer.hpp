#pragma once
#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/dispatcher.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/handler.hpp"

#include "roq/lqs/leg.hpp"
#include "roq/lqs/underlying.hpp"

namespace roq::lqs {

struct Pricer : core::Handler {
  Pricer(core::Dispatcher &dispatcher, core::Manager &core);

  void operator()(const roq::Event<roq::ParametersUpdate> & e) override;
  void operator()(const roq::Event<core::Quotes> &e) override;
  void operator()(const roq::Event<core::ExposureUpdate> &) override;

  std::pair<lqs::Underlying&, bool> emplace_underlying(std::string_view symbol);
  std::pair<lqs::Leg&, bool> emplace_leg(std::string_view symbol, std::string_view exchange);
  
  bool get_underlying(lqs::Leg& leg, std::invocable<lqs::Underlying &> auto fn);
  bool get_leg(core::MarketIdent market, std::invocable<lqs::Leg &> auto fn);

  core::Dispatcher &dispatcher;
  core::Manager &core;
private:
  void dispatch(lqs::Leg const& leg);
  void compute(lqs::Leg& leg, lqs::Underlying const& underlying);
private:  
  core::Hash<roq::Symbol, core::MarketIdent> underlying_by_symbol;
  std::vector<lqs::Underlying> underlyings;
  core::Hash<core::MarketIdent, lqs::Leg> leg_by_market;
};


}

