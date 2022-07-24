#pragma once
#include "umm/prologue.hpp"
#include "umm/context.hpp"
#include "umm/model.hpp"
#include "umm/models/shifter.hpp"
#include "umm/models/mixer.hpp"
#include "umm/models/best_price.hpp"
#include "umm/models/single_quoter.hpp"
#include "umm/quoter.hpp"

namespace umm { 
namespace quoters {


struct CryptoMixerUnstable {
    constexpr static bool UseNoArb = false;
    constexpr static std::string_view name() { return "CryptoMixerUnstable"; }

    template<class Context>
    auto operator()(Context& ctx) {
        auto mdata = ctx % fn::BestPrice();  ///  using BestPrice as marketdata...
        auto legs = [&](auto& mdata, auto leg) {
            return mdata % fn::Mixer(leg, ctx.config);
        };
        return  mdata % fn::Multi(legs, ctx.config) % fn::Shifter(ctx.config) % fn::SingleQuoter();
    }
};

} // quoters
} // umm
