// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/types.hpp"
#include <roq/side.hpp>

namespace roq::core {

struct ExposureKey {
    core::MarketIdent market;
    std::string_view account;
};

struct ExposureValue {
    core::Volume position_buy;
    core::Volume position_sell;
    core::Price  avg_price_buy;
    core::Price  avg_price_sell;
};

struct Exposure {
    core::Volume position_buy;
    core::Volume position_sell;
    core::Price  avg_price_buy;
    core::Price  avg_price_sell;

    operator ExposureValue() {
        return {
            .position_buy = position_buy,
            .position_sell = position_sell,
            .avg_price_buy = avg_price_buy,
            .avg_price_sell = avg_price_sell
        };
    }

    core::MarketIdent market;
    std::string_view account;           // account is exchange-provided and required to route the order

    std::string_view symbol;    
    std::string_view exchange;

    core::PortfolioIdent portfolio;     // portfolio is logical grouping of positions
    std::string_view portfolio_name;
};

} // roq::core

ROQ_CORE_FMT_DECL(roq::core::Exposure, 
    "{{ buy {}@{} sell {}@{} "
    "market.{} {}@{} }}", 
    _.position_buy, _.avg_price_buy,
    _.position_sell, _.avg_price_sell, 
    _.market, _.symbol, _.exchange);
