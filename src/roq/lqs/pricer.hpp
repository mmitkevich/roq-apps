#pragma once
#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/dispatcher.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/handler.hpp"

#include "roq/lqs/leg.hpp"
#include "roq/lqs/underlying.hpp"
#include "roq/lqs/portfolio.hpp"

namespace roq::lqs {

static constexpr std::string_view EXCHANGE = "lqs";



struct Pricer : core::Handler {

  Pricer(core::Dispatcher &dispatcher, core::Manager &core);

  void operator()(const roq::Event<roq::ParametersUpdate> & e) override;
  void operator()(const roq::Event<core::Quotes> &e) override;
  void operator()(const roq::Event<core::ExposureUpdate> &) override;
  
  std::pair<lqs::Portfolio&,bool> emplace_portfolio(std::string_view portfolio_name);
  bool get_portfolio(core::PortfolioIdent portfolio, std::invocable<lqs::Portfolio&> auto fn);
  void get_portfolios(std::invocable<lqs::Portfolio&> auto fn);

  core::Dispatcher &dispatcher;
  core::Manager &core;
private:
  void dispatch(lqs::Leg const& leg);
private:  
  core::Hash<core::PortfolioIdent, lqs::Portfolio> portfolios;
};


inline bool Pricer::get_portfolio(core::PortfolioIdent portfolio, std::invocable<lqs::Portfolio&> auto fn) {
  auto iter = portfolios.find(portfolio);
  if(iter==std::end(portfolios))
    return false;
  fn(iter->second);
  return true;
}

inline void Pricer::get_portfolios(std::invocable<lqs::Portfolio&> auto fn) {
  for(auto&[id, portfolio]: portfolios) {
    fn(portfolio);
  }
}

}

