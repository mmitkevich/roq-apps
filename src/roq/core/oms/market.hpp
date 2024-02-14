#pragma once

#include "roq/core/types.hpp"
#include "roq/string_types.hpp"
#include "roq/core/market.hpp"

namespace roq::core::oms {

struct Market {
    core::MarketIdent market {};
    roq::Symbol symbol;
    roq::Exchange exchange;
    roq::Account account;
    core::PortfolioIdent portfolio {};    
    roq::Account portfolio_name;
    core::StrategyIdent strategy {};


    static oms::Market from(core::Market const& u) {
        return {
            .market = u.market,
            .symbol = u.symbol,
            .exchange = u.exchange,
            .account = u.account,
        };
    }
};

}