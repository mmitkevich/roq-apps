#pragma once
#include "roq/core/shift_units.hpp"
#include "roq/pricer/compute.hpp"

namespace roq::pricer::ops  {

struct QuoteShift : pricer::Compute
{
    struct Parameters {
        core::Shift        min_spread {};
        core::Shift        max_spread {};
    };

    QuoteShift() : Compute(sizeof(Parameters)) {}
    static constexpr std::string_view NAME = "quote_shift";

    bool operator()(pricer::Context &context) const override;
    bool operator()(pricer::Context &context, std::span<const roq::Parameter>  update )  override;
};

} // roq::mmaker