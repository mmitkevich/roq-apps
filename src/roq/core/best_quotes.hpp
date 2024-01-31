#pragma once

#include "roq/core/quote.hpp"
#include <roq/execution_instruction.hpp>
#include <roq/utils/common.hpp>

namespace roq::core {

struct Quotes;

struct BestQuotes {
    core::ExecQuote buy;
    core::ExecQuote sell;
    
    core::ExecQuote &get_quote(roq::Side side);

    const core::ExecQuote& get_quote(roq::Side side) const {
        switch(side) {
            case roq::Side::BUY: return buy;
            case roq::Side::SELL: return sell;
            default: throw roq::RuntimeError("Invalid side");
        }
    }

    void clear();

    bool update(core::Quotes const &rhs);
};


constexpr inline roq::Side multiply(roq::Side lhs, roq::Side rhs) {
    switch(lhs) {
        case roq::Side::BUY: return rhs;
        case roq::Side::SELL: return utils::invert(rhs);
        default: return roq::Side::UNDEFINED;
    }
}

constexpr inline Double pow(core::Double x, roq::Side side) {
    switch(side) {
        case Side::BUY: return x;
        case Side::SELL: return 1.0 / x;
        default: return {};
    }
}

constexpr inline int32_t direction(roq::Side side) {
    switch(side) {
        case Side::BUY: return 1;
        case Side::SELL: return -1;
        default: throw roq::RuntimeError("invalid side");
    }
}

core::Double exposure_by_quotes(core::BestQuotes const &self);

} // roq::ocre

ROQ_CORE_FMT_DECL(roq::core::BestQuotes, "buy_price {} buy_volume {} ask_price {} ask_volume {}", _.buy.price, _.buy.volume, _.sell.price, _.sell.volume)
