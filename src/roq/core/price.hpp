#pragma once

#include "roq/core/empty_value.hpp"
#include "roq/core/types.hpp"
#include <magic_enum.hpp>

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
core::Price mid_price(Quotes const& quotes) {
    if(core::is_empty_value(quotes.buy.price) || core::is_empty_value(quotes.sell.price))
        return {};
    return 0.5*(quotes.buy.price + quotes.sell.price);
}

std::pair<core::Price, core::Price> buy_and_sell_price_from_mid_price_and_spread(core::Price mid_price, core::Shift spread);
core::Price shift_price(core::Price price, core::Shift shift);

}

ROQ_CORE_FMT_DECL(roq::core::ShiftUnits, "{}", magic_enum::enum_name(_))
ROQ_CORE_FMT_DECL(roq::core::Shift, ROQ_CORE_FMT_DOUBLE "{}", _.value, _.units)