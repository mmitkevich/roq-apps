#pragma once

#include "roq/core/price_units.hpp"
#include "roq/core/quotes.hpp"
#include "roq/core/types.hpp"
#include "roq/pricer/manager.hpp"
#include "roq/pricer/node.hpp"
#include "roq/pricer/ops/mid_price.hpp"
#include <roq/exceptions.hpp>
#include <roq/parameters_update.hpp>
#include <roq/pricer/compute.hpp>

namespace roq::pricer::ops {

struct QuoteSpread : pricer::Compute {
    struct Parameters {
        core::PriceUnits        min_spread {};
        core::PriceUnits        max_spread {};
    };

    QuoteSpread() : Compute(sizeof(Parameters)) {}
    static constexpr std::string_view NAME = "target_spread";

    static constexpr auto BP = 1E-4;

    bool operator()(pricer::Context &context) const override;
    bool operator()(pricer::Context &context, std::span<const roq::Parameter>  update )  override;
};


} // roq::pricer