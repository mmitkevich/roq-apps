// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include <roq/client.hpp>

#include "roq/core/types.hpp"
#include "roq/core/quotes.hpp"
#include "roq/core/market_status.hpp"
#include "roq/core/exposure_update.hpp"
#include <roq/parameters_update.hpp>

namespace roq::core {

using Timer = roq::Timer;

struct Handler {
    virtual void operator()(const roq::Event<roq::MarketStatus>& status) {}
    virtual void operator()(const roq::Event<roq::Timer> &) {}
    virtual void operator()(const roq::Event<roq::core::Quotes> &) {}
    virtual void operator()(const roq::Event<roq::core::ExposureUpdate> &) {}
    virtual void operator()(const roq::Event<roq::ParametersUpdate> &) {}  
};

} // roq::core