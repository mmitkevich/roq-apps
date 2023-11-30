#pragma once
#include "roq/core/price.hpp"
#include "roq/core/shift.hpp"
#include "roq/core/types.hpp"
#include "roq/pricer/compute.hpp"

namespace roq::pricer::ops  {

struct QuoteShift : pricer::Compute
{
    struct Parameters {
        //core::Shift         shift_per_unit {};
        //core::Volume        exposure_unit {1.};
    };

    QuoteShift() : Compute(sizeof(Parameters)) {}
    
    static constexpr std::string_view NAME = "quote_shift";

    bool operator()(pricer::Context &context) const override;
    bool operator()(pricer::Context &context, std::span<const roq::Parameter>  update )  override;
};

} // roq::mmaker