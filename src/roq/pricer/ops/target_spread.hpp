#pragma once

#include "roq/core/quotes.hpp"
#include "roq/core/types.hpp"
#include "roq/pricer/manager.hpp"
#include "roq/pricer/node.hpp"
#include "roq/pricer/ops/mid_price.hpp"
#include "roq/pricer/price_units.hpp"
#include <roq/exceptions.hpp>
#include <roq/parameters_update.hpp>

namespace roq::pricer::ops {


struct TargetSpread : pricer::Compute {
    static constexpr std::string_view NAME = "target_spread";

    struct State {
        core::Double target_spread = {};
        pricer::PriceUnits price_units = pricer::PriceUnits::UNDEFINED;
    };
    static constexpr auto BP = 1E-4;

    bool operator()(pricer::Context &context, pricer::Node const& node, pricer::Manager& manager) const override;
    bool operator()(pricer::Context &context, roq::ParametersUpdate const& )  override;
};


} // roq::pricer