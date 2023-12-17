#pragma once

#include "roq/core/price.hpp"
#include "roq/core/quotes.hpp"
#include "roq/core/types.hpp"
#include "roq/dag/manager.hpp"
#include "roq/dag/node.hpp"
#include <roq/exceptions.hpp>
#include <roq/parameters_update.hpp>
#include <roq/dag/compute.hpp>

namespace roq::dag::ops {

struct QuoteSpread : dag::Compute {
    struct Parameters {
        core::Shift        min_spread {};
        core::Shift        max_spread {};
    };

    QuoteSpread() : Compute(sizeof(Parameters)) {}
    static constexpr std::string_view NAME = "quote_spread";

    bool operator()(dag::Context &context) const override;
    bool operator()(dag::Context &context, std::span<const roq::Parameter>  update )  override;
};


} // roq::dag