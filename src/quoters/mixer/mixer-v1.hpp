#pragma once
#include "umm/prologue.hpp"
#include "umm/context.hpp"
#include "umm/model.hpp"
#include "umm/models/shifter.hpp"
#include "umm/models/mixer.hpp"
#include "umm/models/best_price.hpp"
#include "umm/models/single_quoter.hpp"

namespace umm { 
namespace quoters {

template<class Context, class Setup>
auto mixer_v1(Context& ctx, Setup && setup) {
    auto mdata = ctx % fn::BestPrice();  ///  using BestPrice as marketdata...
    auto legs = [&](auto& mdata, auto leg) {
        return mdata % fn::Mixer(leg, ctx.config);
    };
    return  mdata % fn::Multi(legs, ctx.config) % fn::Shifter(ctx.config) % fn::SingleQuoter();
}

}
}
