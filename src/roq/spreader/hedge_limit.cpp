#pragma once
#include "roq/spreader/hedge_limit.hpp"
#include "roq/core/best_quotes.hpp"

namespace roq::spreader {

void HedgeLimit::operator()(roq::Event<roq::ParametersUpdate> const& e) {

}

bool HedgeLimit::operator()(spreader::Spread& spread, std::invocable<spreader::Leg const&> auto fn) {
    bool result = true;

    const core::Double L_B = spread.exec_quotes.buy.price;
    const core::Double L_S = spread.exec_quotes.sell.price;
        
    spread.get_legs([&](spreader::LegIdent bait_leg_id, spreader::Leg& bait_leg) {
        spreader::Leg& hedge_leg = spread.get_other_leg(bait_leg_id);

        if(!hedge_leg.is_local_leg()) {
            return;
        }

        core::BestQuotes& q = hedge_leg.exec_quotes;

        const core::BestQuotes& fills = bait_leg.fills;

        // some fills on leg
        const core::Price F_B = fills.buy.price;
        const core::Volume N_B = fills.buy.volume;

        const core::Price F_S = fills.sell.price;
        const core::Volume N_S = fills.sell.volume;


        if(bait_leg.side == Side::BUY) {
            q.sell.price  = F_B * L_B;
            q.buy.price   = F_S * L_S;
        } else if (bait_leg.side == Side::SELL) {
            q.sell.price  = F_B / L_S;
            q.buy.price   = F_S / L_B;
        }

        core::Double delta_buy = std::max(spread.delta, core::Double{0});
        core::Double delta_sell = std::max(-spread.delta, core::Double{0});

        q.buy.volume  =  delta_sell / hedge_leg.volume_multiplier;
        q.sell.volume =  delta_buy / hedge_leg.volume_multiplier;

        // send out TargetQuotes
        fn(hedge_leg);
    });
    return result;
}


} // roq::spreader