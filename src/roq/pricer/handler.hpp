// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/event.hpp"
#include "roq/core/quotes.hpp"

namespace roq::pricer {

struct Handler {
    virtual void operator()(Event<core::TargetQuotes> const&) = 0;
};

} // roq::oms