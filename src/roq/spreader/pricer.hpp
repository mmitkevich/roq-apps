#pragma once
#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/dispatcher.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/handler.hpp"
#include "roq/spreader/spread.hpp"
#include "roq/spreader/bait.hpp"
#include "roq/spreader/hedge.hpp"

namespace roq::spreader {


//constexpr auto ZERO = core::Double{0.};



struct Pricer : core::Handler {
  Pricer(core::Dispatcher &dispatcher, core::Manager &core);

  void operator()(const roq::Event<roq::ParametersUpdate> & e) override;
  void operator()(const roq::Event<roq::core::Quotes> &e) override;

  void build_spreads();

  bool get_leg(core::MarketIdent market, std::invocable<spreader::Spread &, LegIdent> auto fn);

  template<class T>
  void dispatch(const roq::Event<T>& e) {
    baiter(e);
    hedger(e);
  }

  void dispatch(spreader::Leg const& leg);

  core::Dispatcher &dispatcher;
  core::Manager &core;
private:  
  spreader::Bait baiter;
  spreader::HedgeLimit hedger;

  std::vector<Spread> spreads;
  core::Hash<core::MarketIdent, std::pair<spreader::SpreadIdent,spreader::LegIdent>> leg_by_market; 
  core::Hash<core::PortfolioIdent, spreader::SpreadIdent> spread_by_portfolio;
};


}

