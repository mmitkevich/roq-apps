#pragma once
#include "roq/core/price.hpp"
#include "roq/core/shift.hpp"
#include "roq/core/types.hpp"
#include "roq/dag/compute.hpp"

namespace roq::dag::ops  {

struct QuoteShift : dag::Compute
{
    struct Parameters {
        //core::Shift         shift_per_unit {};
        //core::Volume        exposure_unit {1.};
    };

    QuoteShift() : Compute(sizeof(Parameters)) {}
    
    static constexpr std::string_view NAME = "quote_shift";

    bool operator()(dag::Context &context) const override;
    bool operator()(dag::Context &context, std::span<const roq::Parameter>  update )  override;
};

} // roq::mmaker