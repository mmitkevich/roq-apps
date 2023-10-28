#pragma once

#include "roq/core/quote.hpp"
#include "roq/core/quotes.hpp"

namespace roq::core {

struct BestQuotes {
    core::Quote bid;
    core::Quote ask;

    void clear() {
        bid.clear();
        ask.clear();
    }
    bool update(core::Quotes const& rhs) {
        if(rhs.bids.empty()) {
            bid = {};            
        } else {
            bid = rhs.bids[0];
        }
        if(rhs.asks.empty()) {
            ask = {};
        } else {
            ask = rhs.asks[0];
        }
        return true; // FIXME: return if changed
    }
};

} // roq::ocre