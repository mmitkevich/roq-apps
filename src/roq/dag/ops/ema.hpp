#pragma once

#include "roq/dag/node.hpp"

namespace roq::dag::ops {

struct EMA : Compute {
    struct Parameters {
        core::Double omega {};
    };

    EMA() : Compute(sizeof(Parameters)) {}
    static constexpr std::string_view NAME = "ema";

    bool operator()(dag::Context& context) const override;
    bool operator()(dag::Context &context, std::span<const roq::Parameter> update )  override;
};

} // roq::dag::ops