#pragma once

#include "roq/core/types.hpp"
namespace roq::core {

struct Exposure {
    core::Double exposure;
    core::Double quantity;
    core::PortfolioIdent portfolio;
    core::MarketIdent market;
};

} // roq::core