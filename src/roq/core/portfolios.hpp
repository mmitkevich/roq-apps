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

    void set_position(core::PortfolioIdent portfolio, core::MarketIdent market,
                      core::Volume position);

    core::Volume get_position(core::PortfolioIdent portfolio, core::MarketIdent market, core::Volume position);

    std::pair<core::Portfolio &, bool> emplace_portfolio(core::PortfolioKey key);

    void clear() { portfolios_.clear(); }

    core::PortfolioIdent get_portfolio_ident(std::string_view name);

    

  private:
    core::Manager& core;
    core::PortfolioIdent last_portfolio_id {};
    core::Hash<core::PortfolioIdent, core::Portfolio> portfolios_;
    core::Hash<roq::Account, core::PortfolioIdent> portfolio_index_;
};

} // roq::cache