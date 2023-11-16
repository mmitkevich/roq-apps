#pragma once

#include "roq/pricer/node.hpp"

namespace roq::pricer::ops {

struct EMA : Compute {
    struct Parameters {
        core::Double omega {};
    };

    EMA() : Compute(sizeof(Parameters)) {}
    static constexpr std::string_view NAME = "ema";

    bool operator()(pricer::Context& context) const override;
    bool operator()(pricer::Context &context, std::span<const roq::Parameter> update )  override;
};

} // roq::pricer::ops