// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/event.hpp"
#include "roq/core/trade.hpp"


namespace roq::core::oms {
    
struct Handler {
    //virtual void operator()(core::ExposureUpdate const&, oms::Manager& source) = 0;
    virtual void operator()(core::Trade const& trade) = 0; // from fills
};

} // roq::oms