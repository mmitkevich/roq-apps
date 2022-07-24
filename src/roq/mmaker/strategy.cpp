#include "roq/api.hpp"
#include "strategy.hpp"
#include <roq/client.hpp>

namespace roq {
namespace mmaker {
using namespace umm::literals;

Strategy::Strategy(client::Dispatcher& dispatcher, Context& context)
: dispatcher_(dispatcher)
, context_(context)
, cache_(client::MarketByPriceFactory::create)
{
    quoter_ = context_.factory(context);
}

template<class T> 
cache::Market& Strategy::update_market(const Event<T> &event) {
    auto [market, is_new] = cache_.get_market_or_create(event.value.exchange, event.value.symbol);
    bool done = market(event);
    return market;
}

void Strategy::operator()(const Event<Timer> &event) {
  
}

void Strategy::operator()(const Event<Connected> &event) {
  
}

void Strategy::operator()(const Event<Disconnected> &event) {
  
}

void Strategy::operator()(const Event<DownloadBegin> &event) {
  
}

void Strategy::operator()(const Event<DownloadEnd> &event) {
  
}

void Strategy::operator()(const Event<GatewayStatus> &event) {
  
}

void Strategy::operator()(const Event<ReferenceData> &event) {
  context_(event, update_market(event));
}

void Strategy::operator()(const Event<MarketStatus> &event) {
  update_market(event);
}

void  Strategy::operator()(const Event<TopOfBook> &) {

}

void Strategy::operator()(const Event<MarketByPriceUpdate> &event) {
    if(context_(event, update_market(event))) {
      auto umm_id = context_.get_market_ident(event);
      log::info<1>("quoter update symbol={}, exchange={}, umm={} {}", event.value.symbol, event.value.exchange, umm_id.value, context_.markets(umm_id));
      quoter_->update(umm_id, "Quotes"_id);
    }
}

void Strategy::operator()(const Event<OrderAck> &event) {

}

void Strategy::operator()(const Event<OrderUpdate> &event) {
 
}

void Strategy::operator()(const Event<TradeUpdate> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<PositionUpdate> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<FundsUpdate> &event) {
}

void Strategy::operator()(const Event<RateLimitTrigger> &event) {

}

void Strategy::operator()(metrics::Writer &) const {

}

} // mmaker
} // roq