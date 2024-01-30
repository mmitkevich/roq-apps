// (c) copyright 2023 Mikhail Mitkevich
#pragma once

namespace roq::core {

enum class BestQuotesSource {
    UNDEFINED,
    TOP_OF_BOOK,
    MARKET_BY_PRICE,
    VWAP
};

} // roq::core