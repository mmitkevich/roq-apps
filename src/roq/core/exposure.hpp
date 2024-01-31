// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/types.hpp"
#include <roq/side.hpp>

namespace roq::core {

struct Exposure {
    //roq::Side side {Side::UNDEFINED}; // BUY, SELL
    //core::Double price {NAN};
    //core::Double quantity {NAN}; // size of the fill it is delta(exposure) single partial fill (one order)
    //core::Double exposure {NAN}; // cum qty (for 1 order) -- could be used for checking
    core::Volume position_buy;
    core::Volume position_sell;
    core::Price  avg_price_buy;
    core::Price  avg_price_sell;

    core::MarketIdent market;
    std::string_view exchange;
    std::string_view symbol;

    core::PortfolioIdent portfolio; 
    std::string_view portfolio_name;
};

} // roq::core

ROQ_CORE_FMT_DECL(roq::core::Exposure, 
    "{{ buy {}@{} sell {}@{} "
    "market.{} {}@{}  "
    "portfolio.{} {} }}", 
    _.position_buy, _.avg_price_buy,
    _.position_sell, _.avg_price_sell, 
    _.market, _.symbol, _.exchange,  
    _.portfolio, _.portfolio_name);
