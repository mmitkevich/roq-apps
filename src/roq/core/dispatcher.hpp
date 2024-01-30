// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/event.hpp"
#include "roq/core/quotes.hpp"

namespace roq::core {

struct Dispatcher {
    virtual void operator()(core::TargetQuotes const&) = 0;
};

} // roq::oms