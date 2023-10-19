#pragma once
#include "roq/core/types.hpp"
#include "roq/core/quotes.hpp"
#include "roq/core/market_status.hpp"
#include "roq/core/exposure_update.hpp"
namespace roq::core {

using Timer = roq::Timer;

struct Handler {
    virtual void operator()(const roq::Event<roq::MarketStatus>& status) = 0;
    virtual void operator()(const roq::Event<roq::Timer> &) = 0;
    virtual void operator()(const roq::Event<roq::core::Quotes> &) = 0;
    virtual void operator()(const roq::Event<roq::core::ExposureUpdate> &) = 0;    
};

} // roq::core