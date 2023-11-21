#pragma once

#include "roq/core/empty_value.hpp"
#include "roq/core/types.hpp"

namespace roq::core {
    
enum class ShiftUnits {
    UNDEFINED = 0,
    PRICE = 1,
    BP = 2,
    TICKS = 3,
    VOLATILITY = 4
};

struct Shift : core::Price {
    using Price::Price;
    core::ShiftUnits units;

    static Shift parse(std::string_view s, core::ShiftUnits dflt = core::ShiftUnits::PRICE);
};

template<class Quotes>
core::Price MidPrice(Quotes const& quotes) {
    if(core::is_empty_value(quotes.buy.price) || core::is_empty_value(quotes.sell.price))
        return {};
    return 0.5*(quotes.buy.price + quotes.sell.price);
}

std::pair<core::Price, core::Price> Spread(core::Shift spread, core::Price mid_price);

}