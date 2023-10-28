#pragma once

#include "roq/core/empty_value.hpp"
#include "roq/core/types.hpp"

namespace roq::pricer::ops {
    
template<class Quotes>
core::Price MidPrice(Quotes const& quotes) {
    if(core::is_empty_value(quotes.bid.price) || core::is_empty_value(quotes.ask.price))
        return {};
    return 0.5*(quotes.bid.price + quotes.ask.price);
}

}