#pragma once

#include "roq/core/double.hpp"
#include "roq/core/types.hpp"

namespace roq::core {

enum class Units {
    UNDEFINED = 0,
    PRICE = 1,
    BP = 2,
    TICKS = 3,
    VOLATILITY = 4
};

struct PriceUnits : core::Price {
    using Price::Price;
    core::Units units;

    static PriceUnits parse(std::string_view s, core::Units dflt = Units::PRICE);
};

} // roq::pricer