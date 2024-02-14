#pragma once

#include "roq/core/types.hpp"
#include "roq/string_types.hpp"
#include "roq/core/market.hpp"
#include "roq/core/market/info.hpp"

namespace roq::core::oms {

struct Market {
    core::MarketIdent market {};
    roq::Symbol symbol;
    roq::Exchange exchange;
    roq::Account account;
    core::PortfolioIdent portfolio {};    
    roq::Account portfolio_name;
    core::StrategyIdent strategy {};
    roq::Source trade_gateway_name {};


    //oms::Market& merge(core::Market const& u) {
    //    market = u.market;
    //    symbol = u.symbol;
    //    exchange = u.exchange;
    //    return *this;
    //}
    oms::Market& merge(market::Info const& u) {
        market = u.market;
        symbol = u.symbol;
        exchange = u.exchange;
        trade_gateway_name = u.mdata_gateway_name;
        return *this;
    }

};

}