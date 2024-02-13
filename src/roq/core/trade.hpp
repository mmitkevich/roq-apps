#pragma once
#include <string_view>
#include "roq/core/types.hpp"
#include "roq/side.hpp"
#include "roq/numbers.hpp"

namespace roq::core {


struct Trade {
    roq::Side side = {};
    double price = NaN;
    double quantity = NaN;
    core::MarketIdent market;
    std::string_view symbol;
    std::string_view exchange;
    core::PortfolioIdent portfolio {};
    std::string_view account;
};

} // roq::core