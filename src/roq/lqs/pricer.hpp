#pragma once

#include "roq/core/best_quotes.hpp"
#include "roq/core/dispatcher.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/handler.hpp"

//#include "roq/core/basic_pricer.hpp"

namespace roq::lqs {

using LegIdent = uint32_t;

using SpreadIdent = uint32_t;

struct Leg {
  roq::Side side {};  // side of the leg
  core::Double delta_by_volume {1.}; // from volume to delta
  core::BestQuotes exec_quotes;
  core::BestQuotes market_quotes;
  core::Volume position;
};

// buy_1 = bid_2 * M
// sell_1 = ask_2 * M
// buy_2 = bid_1 / M
// sell_2 = ask_1 / M

struct Spread : Leg {
  std::array<Leg,2> legs;
  core::PortfolioIdent portfolio;
  core::BestQuotes exec_quotes;     // of the spread
  core::BestQuotes market_quotes;
  core::Double min_delta, max_delta;
  core::Double target_delta;

  Leg& get_leg(LegIdent leg_id) {
    assert(leg_id<legs.size());
    return legs[leg_id];
  }
  
  Leg& get_hedge_leg(LegIdent leg_id) {
    assert(leg_id<2);
    return legs[(1-leg_id)];
  }  
};

constexpr auto ZERO = core::Double{0.};

struct Pricer : core::Handler {
  Pricer(core::Dispatcher &dispatcher, core::Manager &core);

  bool compute(core::MarketIdent market, std::invocable<core::Quotes const&> auto fn);

  void operator()(const roq::Event<roq::core::Quotes> &e) override;

  void build_spreads();

  bool get_leg(core::MarketIdent market, std::invocable<lqs::Spread &, LegIdent> auto fn);

  core::Dispatcher &dispatcher;
  core::Manager &core;
  
  std::vector<Spread> spreads;
  core::Hash<core::MarketIdent, std::pair<lqs::SpreadIdent,lqs::LegIdent>> leg_by_market; 
  core::Hash<core::PortfolioIdent, lqs::SpreadIdent> spread_by_portfolio;
};


}

