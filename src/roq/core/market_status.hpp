// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/types.hpp"
#include "roq/trading_status.hpp"

namespace roq::core {

struct MarketStatus {
    core::MarketIdent market = {};
    TradingStatus trading_status = {};  //!< Trading status
};

}