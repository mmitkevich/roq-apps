#pragma once
#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/dispatcher.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/handler.hpp"
#include "roq/lqs/spread.hpp"
//#include "roq/core/basic_pricer.hpp"
#include "roq/lqs/baiter.hpp"
#include "roq/lqs/hedger.hpp"

namespace roq::lqs {


//constexpr auto ZERO = core::Double{0.};



struct Pricer : core::Handler {
  Pricer(core::Dispatcher &dispatcher, core::Manager &core);

  void operator()(const roq::Event<roq::core::Quotes> &e) override;

  void build_spreads();

  bool get_leg(core::MarketIdent market, std::invocable<lqs::Spread &, LegIdent> auto fn);

private:
  void dispatch(lqs::Leg const& leg);

  core::Dispatcher &dispatcher;
  core::Manager &core;
  
  lqs::Baiter baiter;
  lqs::Hedger hedger;

  std::vector<Spread> spreads;
  core::Hash<core::MarketIdent, std::pair<lqs::SpreadIdent,lqs::LegIdent>> leg_by_market; 
  core::Hash<core::PortfolioIdent, lqs::SpreadIdent> spread_by_portfolio;
};


}

