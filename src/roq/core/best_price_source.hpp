#pragma once

namespace roq::core {

enum class BestPriceSource {
    UNDEFINED,
    TOP_OF_BOOK,
    MARKET_BY_PRICE,
    VWAP
};

} // roq::core