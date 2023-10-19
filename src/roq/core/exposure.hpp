#pragma once

#include "roq/core/types.hpp"
#include <roq/side.hpp>

namespace roq::core {

struct Exposure {
    roq::Side side {Side::UNDEFINED};
    core::Double price {NAN};
    core::Double quantity {NAN};
    core::Double exposure {NAN};
    
    std::string_view exchange;
    std::string_view symbol;
    core::MarketIdent market;

    core::PortfolioIdent portfolio;
    std::string_view account;    
};

} // roq::core