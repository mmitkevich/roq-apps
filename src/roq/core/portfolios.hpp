// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include "roq/core/types.hpp"
#include "roq/event.hpp"
#include "roq/core/portfolio.hpp"
#include "roq/core/exposure_update.hpp"

namespace roq::core {

struct Portfolios {

    void operator()(const roq::Event<core::ExposureUpdate>& event);

    void set_position(core::PortfolioIdent portfolio, core::MarketIdent market, core::Volume position) {
        portfolios_[portfolio].set_position(market, position);
    }
    
    core::Volume get_position(core::PortfolioIdent portfolio, core::MarketIdent market, core::Volume position) {
        return portfolios_[portfolio].get_position(market);
    }

    std::pair<core::Portfolio &, bool> emplace_portfolio(core::PortfolioKey key);

    void clear() { portfolios_.clear(); }
private:
    core::PortfolioIdent last_portfolio_id {};
    core::Hash<core::PortfolioIdent, core::Portfolio> portfolios_;
    core::Hash<roq::Account, core::PortfolioIdent> portfolio_index_;
};

} // roq::cache