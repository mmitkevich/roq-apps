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

    core::Portfolio& emplace_portfolio(core::PortfolioIdent ident) { return portfolios_[ident]; }

    void clear() { portfolios_.clear(); }
private:
    core::Hash<core::PortfolioIdent, core::Portfolio> portfolios_;
};

} // roq::cache