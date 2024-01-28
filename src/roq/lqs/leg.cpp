#include "roq/lqs/leg.hpp"

namespace roq::lqs {

void Leg::compute(lqs::Underlying const& u, lqs::Pricer& pricer) {
    core::BestQuotes& q = this->exec_quotes;
    core::BestQuotes& m = this->market_quotes;            
    core::BestQuotes& p = this->position;
 
    core::Double bid_ask_spread = ( m.sell.price/m.buy.price - 1.0 ) * 1E4;

    if(m.buy.empty() || m.sell.empty() || bid_ask_spread > slippage) {
        exec_quotes.clear();
        return; // don't liquidate if we have wide spread or one-sided quotes
    }
    
    // take market agressively
    q.buy.price = m.sell.price;
    q.sell.price = m.buy.price;

    // exposure = signed leg position
    core::Volume exposure = p.buy.volume - p.sell.volume;

    // trade towards zero exposure
    q.buy.volume = buy_volume.min((-exposure).max(0));
    q.sell.volume = sell_volume.min((exposure).max(0));

    // avoid delta leaving delta_range
    q.buy.volume = q.buy.volume.min( (u.delta_max-u.delta).max(0)/delta_by_volume );
    q.sell.volume = q.sell.volume.min( (u.delta-u.delta_min).max(0)/delta_by_volume );
}

} // roq::lqs