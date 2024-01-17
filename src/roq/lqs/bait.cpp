#include "roq/lqs/bait.hpp"

namespace roq::lqs {

bool Bait::operator()(lqs::Spread& spread, std::invocable<lqs::Leg const&> auto fn) {
    bool result = true;

    const core::Double delta = spread.delta;
    const core::Double L_B = spread.exec_quotes.buy.price;
    const core::Double L_S = spread.exec_quotes.sell.price;
    
    spread.get_legs([&](lqs::LegIdent bait_leg_id, lqs::Leg& bait_leg) {
        if(!bait_leg.is_local_leg()) {
            return;
        }
        lqs::Leg& hedge_leg = spread.get_other_leg(bait_leg_id);

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

        q.buy.volume =  delta_B / bait_leg.delta_by_volume;
        q.sell.volume = delta_S / bait_leg.delta_by_volume;

        // send out TargetQuotes
        fn(bait_leg);
    });

    return result;
}


} // roq::lqs