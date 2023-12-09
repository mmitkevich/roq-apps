#pragma once

#include "roq/core/quote.hpp"
#include "roq/core/quotes.hpp"

namespace roq::core {

struct BestQuotes {
    core::Quote buy;
    core::Quote sell;

    void clear() {
        buy.clear();
        sell.clear();
    }

    bool update(core::Quotes const& rhs) {
        if(rhs.buy.empty()) {
            buy = {};            
        } else {
            buy = rhs.buy[0];
        }
        if(rhs.sell.empty()) {
            sell = {};
        } else {
            sell = rhs.sell[0];
        }
        return true; // FIXME: return if changed
    }
};

inline core::Double ExposureFromQuotes(core::BestQuotes const& self) {
    return self.buy.volume - self.sell.volume;
}


} // roq::ocre

ROQ_CORE_FMT_DECL(roq::core::BestQuotes, "buy_price {} buy_volume {} ask_price {} ask_volume {}", _.buy.price, _.buy.volume, _.sell.price, _.sell.volume)
