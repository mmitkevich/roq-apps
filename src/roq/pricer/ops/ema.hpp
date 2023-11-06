#pragma once

#include "roq/pricer/node.hpp"

namespace roq::pricer::ops {

struct EMA : Compute {
    struct State {
        core::Double omega {};
    };

    EMA() {
        this->size = sizeof(State);
    }
    
    static constexpr std::string_view NAME = "ema";
    bool operator()(pricer::Context& context, pricer::Node const& node, pricer::Manager & manager) const override;
};

} // roq::pricer::ops