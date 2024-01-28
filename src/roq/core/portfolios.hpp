// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include "roq/core/types.hpp"
#include "roq/event.hpp"
#include "roq/core/portfolio.hpp"
#include "roq/core/exposure_update.hpp"
#include <roq/position_update.hpp>

namespace roq::core {
struct Manager;

struct Portfolios {

    Portfolios(core::Manager& core) 
    : core(core) {}
    
    void operator()(const roq::Event<core::ExposureUpdate>& event);
    
    void operator()(const roq::Event<roq::PositionUpdate>& event);

    core::Volume get_net_exposure(core::PortfolioIdent portfolio, core::MarketIdent market, core::Volume exposure = {});

    std::pair<core::Portfolio &, bool> emplace_portfolio(core::PortfolioKey key);

    void clear() { portfolios_.clear(); }

    core::PortfolioIdent get_portfolio_ident(std::string_view name);

    // enumerate portfolios
    void get_portfolios(std::invocable<core::Portfolio const&> auto callback) {
      for(auto& [portfolio_id, portfolio]:portfolios_) {
        callback(portfolio);
      }
    }

    // enumerate exposure for specific portfolio
    void get_exposures(core::PortfolioIdent portfolio_id, std::invocable<core::Exposure const&> auto callback) {
      auto iter = portfolios_.find(portfolio_id);
      core::Portfolio& portfolio = iter->second;
      portfolio.get_positions(callback);
    }

    // enumerate exposure in all portfolios
    void get_exposures(std::invocable<core::Exposure const&> auto callback) {
      for(auto& [portfolio_id, portfolio] : portfolios_) {
        get_exposures(portfolio_id, callback);
      }
    }


  private:
    core::Manager& core;
    core::PortfolioIdent last_portfolio_id {};
    core::Hash<core::PortfolioIdent, core::Portfolio> portfolios_;
    core::Hash<roq::Account, core::PortfolioIdent> portfolio_index_;
};

} // roq::cache