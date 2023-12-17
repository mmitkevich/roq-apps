#pragma once

#include "roq/core/basic_pricer.hpp"

namespace roq::lqs {

struct Manager : core::BasicPricer<Manager, core::Handler> {
    using Base = core::BasicPricer<Manager, core::Handler>;
    using Base::Base;

    void compute(core::Quote& buy, core::Quote& sell);
};
}

