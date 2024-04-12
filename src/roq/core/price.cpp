#include "roq/core/price.hpp"
#include "roq/core/string_utils.hpp"
#include <cctype>
#include <charconv>
#include <roq/exceptions.hpp>


namespace roq::core {

static constexpr auto BP = 1E-4;

core::Shift Shift::parse(std::string_view s, core::ShiftUnits dflt/* = ShiftUnits::PRICE*/) {
    using namespace std::literals;

    core::Shift result {};
    result.units = dflt;
    
    /*auto [p, ec] = std::from_chars(s.data(), s.data()+s.size(), result.value);

    if (ec != std::errc()) {
        throw roq::RuntimeError("Invalid Shift {}", s);
    }*/
    
    result.value = core::to_double(s);
    
    // NOTE: drop support for BP/TICKS for now due to clang++

/*
    auto units = core::trim_left(std::string_view(p, s.data()+s.size()-p), " "sv);
    if(units=="BP"sv) {
        result.units = ShiftUnits::BP;
    } else if(units=="TICK"sv) {
        result.units = ShiftUnits::TICKS;
    }
*/    
    return result;
}

core::Price shift_price(core::Price price, core::Shift shift) {
    switch (shift.units) {
        case core::ShiftUnits::BP: {
            return shift < 0 ? 
                price / (1. + shift * BP) : 
                price * (1. + shift * BP);
        } break;
        case core::ShiftUnits::PRICE: {
            return price + 0.5 * shift;
        } break;
        default: {
            throw roq::RuntimeError("UNEXPECTED");
        }
    }
}  

std::pair<core::Price, core::Price> buy_and_sell_price_from_mid_price_and_spread(core::Price mid_price, core::Shift spread) {
    core::Price buy_price {}, sell_price {};

    switch (spread.units) {
        case core::ShiftUnits::BP: {
            buy_price = mid_price / (1. + .5 * spread * BP);
            sell_price = mid_price * (1. + .5 * spread * BP);
        } break;
        case core::ShiftUnits::PRICE: {
            buy_price = mid_price - 0.5 * spread;
            sell_price = mid_price + 0.5 * spread;
        } break;
        default: {
            throw roq::RuntimeError("UNEXPECTED");
        }
    }
    return {buy_price, sell_price};
}



} // roq::core