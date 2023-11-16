// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/types.hpp"
#include <roq/side.hpp>

namespace roq::core {

struct Exposure {
    roq::Side side {Side::UNDEFINED}; // BUY, SELL
    core::Double price {NAN};
    core::Double quantity {NAN}; // size of the fill it is delta(exposure) single partial fill (one order)
    core::Double exposure {NAN}; // cum qty (for 1 order) -- could be used for checking
    
    std::string_view exchange;
    std::string_view symbol;
    core::MarketIdent market;

    core::PortfolioIdent portfolio;
    std::string_view account;    
};

} // roq::core