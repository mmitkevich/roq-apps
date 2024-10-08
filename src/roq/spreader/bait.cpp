#include "roq/spreader/bait.hpp"

namespace roq::spreader {

void Bait::operator()(roq::Event<roq::ParametersUpdate> const& e) {
    
}

bool Bait::operator()(spreader::Spread& spread, std::invocable<spreader::Leg const&> auto fn) {
    bool result = true;

    const core::Double delta = spread.delta;
    const core::Double L_B = spread.exec_quotes.buy.price;
    const core::Double L_S = spread.exec_quotes.sell.price;
    
    spread.get_legs([&](spreader::LegIdent bait_leg_id, spreader::Leg& bait_leg) {
        if(!bait_leg.is_local_leg()) {
            return;
        }
        spreader::Leg& hedge_leg = spread.get_other_leg(bait_leg_id);

        core::BestQuotes& q = bait_leg.exec_quotes;

        const core::Double M_B = hedge_leg.market_quotes.buy.price;
        const core::Double M_S = hedge_leg.market_quotes.buy.price;

        const core::Double N = bait_leg.exposure;

        // delta_range.min = -$100
        // delta_range.max = $100

        const core::Double delta_B = std::min(std::max(-N, core::Double{0.}), spread.delta_range.max-delta); // anyway if filled this order my delta will not exceed delta.max
        const core::Double delta_S = std::min(std::max(N, core::Double{0.}), delta-spread.delta_range.min);

        // d_l == bait_leg.side
        if(bait_leg.side==Side::BUY) { 
            q.buy.price  = M_B * L_B;
            q.sell.price =  M_B * L_S;
        } else if (bait_leg.side==Side::SELL) {         
            q.buy.price = M_S / L_S;
            q.sell.price = M_S / L_B;
        } else {
            assert(false);
        }

        q.buy.volume =  delta_B / bait_leg.volume_multiplier;
        q.sell.volume = delta_S / bait_leg.volume_multiplier;

        // send out TargetQuotes
        fn(bait_leg);
    });

    return result;
}


} // roq::spreader