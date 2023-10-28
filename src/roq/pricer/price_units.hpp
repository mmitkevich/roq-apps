#pragma once

namespace roq::pricer {

enum class PriceUnits {
    UNDEFINED = 0,
    PRICE = 1,
    BP = 2,
    TICKS = 3,
    VOLATILITY = 4
};

} // roq::pricer