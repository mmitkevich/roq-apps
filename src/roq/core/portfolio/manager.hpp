// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include "roq/core/types.hpp"
#include "roq/event.hpp"
#include "roq/core/portfolio.hpp"
//#include "roq/core/manager.hpp"
#include "roq/core/exposure_update.hpp"
#include <roq/position_update.hpp>
#include "roq/core/config/toml_file.hpp"
#include "roq/parameters_update.hpp"
#include "roq/order_update.hpp"
#include "roq/core/position_source.hpp"

namespace roq::core::portfolio {

struct Manager;

struct Handler {
  virtual void operator()(core::ExposureUpdate const& u) = 0;
};


struct Manager {

    Manager(roq::core::Manager& core) 
    : core{core} {}
    
    void set_handler(portfolio::Handler* h) {
      this->handler = h;
    }

    // config::Handler
    //void operator()(roq::Event<roq::ParametersUpdate> const& event);

    // oms::Handler
    void operator()(core::Trade const & trade);
    
    // client::Handler
    void operator()(const roq::Event<roq::PositionUpdate>& event);


    //core::Volume get_net_exposure(core::PortfolioIdent portfolio, core::MarketIdent market, core::Volume exposure = {});

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
    void get_positions(core::PortfolioIdent portfolio_id, std::invocable<core::ExposureUpdate const&> auto callback) {
      auto iter = portfolios_.find(portfolio_id);
      core::Portfolio& portfolio = iter->second;
      portfolio.get_positions([&](core::Exposure const& exposure) {
        core::ExposureUpdate update {
          .exposure = std::span {&exposure, 1},
          .portfolio = portfolio.portfolio,
          .portfolio_name = portfolio.portfolio_name,
        };
        callback(update);
      });
    }

    // enumerate exposure in all portfolios
    void get_positions(std::invocable<core::ExposureUpdate const&> auto callback) {
      for(auto& [portfolio_id, portfolio] : portfolios_) {
        this->get_positions(portfolio_id, callback);
      }
    }

    void configure(const config::TomlFile& config, config::TomlNode root);

    bool get_portfolio_by_account(std::string_view account, std::string_view exchange, std::invocable<core::Portfolio&> auto callback) {
        auto & by_account = portfolio_by_account_[exchange];
        auto iter = by_account.find(account);
        if( iter != std::end(by_account) ) {
          core::PortfolioIdent portfolio_id = iter->second;
          auto iter_2 = portfolios_.find(portfolio_id);
          if(iter_2!=std::end(portfolios_)) {
            callback(iter_2->second);
            return true;
          }
        }
        return false;
    }
  public:
    core::PositionSource position_source {core::PositionSource::PORTFOLIO};
  private:
    portfolio::Handler* handler = nullptr; // notify about exposure change
    core::Manager& core;
    core::PortfolioIdent last_portfolio_id {};
    core::Hash<core::PortfolioIdent, core::Portfolio> portfolios_;
    core::Hash<roq::Account, core::PortfolioIdent> portfolio_by_name_;
    core::Hash<roq::Exchange, core::Hash<roq::Account, core::PortfolioIdent>> portfolio_by_account_;
};



} // roq::core