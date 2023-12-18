#pragma once

#include "roq/core/quote.hpp"

namespace roq::core {

struct Quotes;

struct BestQuotes {
    core::Quote buy;
    core::Quote sell;

    void clear();

    bool update(core::Quotes const &rhs);
};

core::Double ExposureFromQuotes(core::BestQuotes const &self);

} // roq::ocre

ROQ_CORE_FMT_DECL(roq::core::BestQuotes, "buy_price {} buy_volume {} ask_price {} ask_volume {}", _.buy.price, _.buy.volume, _.sell.price, _.sell.volume)
