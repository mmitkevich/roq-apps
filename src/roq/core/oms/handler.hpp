// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/event.hpp"
#include "roq/core/exposure_update.hpp"

namespace roq::core::oms {
struct Manager;

struct Handler {
    virtual void operator()(core::ExposureUpdate const&, oms::Manager& source) = 0;
};

} // roq::oms