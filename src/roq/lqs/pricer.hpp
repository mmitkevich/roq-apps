#pragma once
#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/dispatcher.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/handler.hpp"

#include "roq/lqs/leg.hpp"
#include "roq/lqs/underlying.hpp"
#include "roq/lqs/strategy.hpp"

namespace roq::lqs {

static constexpr std::string_view EXCHANGE = "lqs";



struct Pricer : core::Handler {

  Pricer(core::Dispatcher &dispatcher, core::Manager &core);

  void operator()(const roq::Event<roq::ParametersUpdate> & e) override;
  void operator()(const roq::Event<core::Quotes> &e) override;
  void operator()(const roq::Event<core::ExposureUpdate> &) override;
  void operator()(const roq::Event<roq::ReferenceData> &) override;
  
  void operator()(const roq::Event<roq::DownloadBegin> &) override;
  void operator()(const roq::Event<roq::DownloadEnd> &) override;
  
  std::pair<lqs::Strategy&,bool> emplace_strategy(core::StrategyIdent strategy_id);
  bool get_strategy(core::PortfolioIdent portfolio, std::invocable<lqs::Strategy&> auto fn);
  void get_strategies(std::invocable<lqs::Strategy&> auto fn);
  void dispatch(lqs::Leg const& leg, lqs::Strategy const& strategy);

  core::Dispatcher &dispatcher;
  core::Manager &core;
private:  
  core::Hash<core::StrategyIdent, lqs::Strategy> strategies_;
};


inline bool Pricer::get_strategy(core::StrategyIdent strategy_id, std::invocable<lqs::Strategy&> auto fn) {
  auto iter = strategies_.find(strategy_id);
  if(iter==std::end(strategies_))
    return false;
  fn(iter->second);
  return true;
}

inline void Pricer::get_strategies(std::invocable<lqs::Strategy&> auto fn) {
  for(auto&[id, strategy]: strategies_) {
    fn(strategy);
  }
}

}

