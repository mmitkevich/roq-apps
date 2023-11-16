#include "roq/core/price_units.hpp"
#include "roq/core/string_utils.hpp"
#include <cctype>
#include <charconv>
#include <roq/exceptions.hpp>


namespace roq::core {

core::PriceUnits PriceUnits::parse(std::string_view s, core::Units dflt/* = Units::PRICE*/) {
    using namespace std::literals;

    core::PriceUnits result {};
    result.units = dflt;
    
    auto [p, ec] = std::from_chars(s.data(), s.data()+s.size(), result.value);

    if (ec != std::errc()) {
        throw roq::RuntimeError("Invalid PriceUnits {}", s);
    }
    auto units = core::trim_left(std::string_view(p, s.data()+s.size()-p), " "sv);
    if(units=="BP"sv) {
        result.units = Units::BP;
    } else if(units=="PRICE"sv ) {
        result.units = Units::PRICE;
    } else if(units=="TICK"sv) {
        result.units = Units::TICKS;
    }
    return result;
}


} // roq::core