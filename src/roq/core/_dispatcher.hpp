#pragma once 

#include "roq/core/quotes.hpp"

namespace roq::core {

struct Dispatcher {
    virtual void send(core::TargetQuotes const&) = 0; // "target quotes"
};


} // roq::core