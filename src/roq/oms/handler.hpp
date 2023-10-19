#pragma once

#include "roq/event.hpp"
#include "roq/core/exposure_update.hpp"

namespace roq::oms {

struct Handler {
    virtual void operator()(Event<core::ExposureUpdate> const&) = 0;
};

} // roq::oms