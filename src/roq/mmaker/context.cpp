#include "context.hpp"
#include "umm/empty_value.hpp"
#include "umm/type.hpp"
#include "umm/type/best_price.hpp"
#include <roq/cache/market_by_price.hpp>

namespace roq { 
namespace mmaker {


void Context::operator()(const Event<MarketByPriceUpdate>& event, cache::Market& market) {
    auto umm_id = get_umm_id(market);
    auto bids = market.market_by_price->bids();
    auto asks = market.market_by_price->asks();
    auto& mbp = *market.market_by_price;
    umm::BestPrice best_price;
    if(!bids.empty()) {
        best_price.bid_price = mbp.internal_to_price(bids[0].first);
        best_price.bid_volume = mbp.internal_to_price(bids[0].second);
    }
    if(!asks.empty()) {
        best_price.ask_price = mbp.internal_to_price(asks[0].first);
        best_price.ask_volume = mbp.internal_to_price(asks[0].second);
    }
    umm.best_price[umm_id] = best_price;
}

void Context::operator()(const Event<ReferenceData> &event, roq::cache::Market& market) {
    auto & src = market.reference_data;
    auto umm_id = get_umm_id(market);
    umm.tick_rules.min_trade_vol[umm_id] =  market.reference_data.min_trade_vol;
    umm.tick_rules.tick_size[umm_id] =  market.reference_data.tick_size;
}

} // mmaker
} // roq