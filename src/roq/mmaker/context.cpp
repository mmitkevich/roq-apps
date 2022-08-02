#include "context.hpp"
#include "umm/empty_value.hpp"
#include "umm/type.hpp"
#include "umm/type/best_price.hpp"
#include <roq/cache/market_by_price.hpp>
#include <roq/client/config.hpp>
#include <roq/order_cancel_policy.hpp>
#include <roq/order_management.hpp>
#include <roq/logging.hpp>

namespace roq { 
namespace mmaker {

using  namespace std::literals;

Context::Context() {
}

/// Config::Handler
void Context::dispatch(Handler &handler) const {
    //log::info<1>("Config::dispatch"sv);
    markets_map_.get_markets([&](const auto& item) {
        log::info<1>("symbol={}, exchange={}, market {}"sv, item.symbol, item.exchange, markets(item.market));
        handler(client::Symbol {
            .regex = item.symbol,
            .exchange = item.exchange
        });
    });

    for(auto& account_str: accounts_) {
        log::info<1>("account={}"sv, account_str);
        handler(client::Account {
            .regex = account_str
        });
    };
}

umm::BestPrice Context::get_best_price_from_market_by_price(const cache::Market& market) {
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
    return best_price;
}

bool Context::operator()(const Event<MarketByPriceUpdate>& event, cache::Market& data) {
    auto market = get_market_ident(data);
    if(best_price_source==BestPriceSource::MARKET_BY_PRICE) {
        this->best_price[market] = get_best_price_from_market_by_price(data);

    } else if(best_price_source==BestPriceSource::VWAP) {
        // TODO
    } else {
        return false;
    }
    return true;
}

bool Context::operator()(const Event<ReferenceData> &event, roq::cache::Market& data) {
    auto& refdata = data.reference_data;
    auto market = get_market_ident(data);
    this->tick_rules.min_trade_vol[market] =  refdata.min_trade_vol;
    this->tick_rules.tick_size[market] =  refdata.tick_size;
    return true;
}

} // mmaker
} // roq