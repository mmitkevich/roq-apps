#include "roq/api.hpp"
#include "roq/mmaker/best_price.hpp"
#include "strategy.hpp"
#include "best_price.hpp"

namespace roq {
namespace mmaker {


Strategy::Strategy(client::Dispatcher& dispatcher, Context& context)
: dispatcher_(dispatcher)
, context_(context)
{}

template<class T> 
cache::Market& Strategy::update_market(const Event<T> &event) {
    auto [market, is_new] = cache_.get_market_or_create(event.value.exchange, event.value.symbol);
    market(event);
    return market;
}

void Strategy::operator()(const Event<Timer> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<Connected> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<Disconnected> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<DownloadBegin> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<DownloadEnd> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<GatewayStatus> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<ReferenceData> &event) {
  context_(event, update_market(event));
}

void Strategy::operator()(const Event<MarketStatus> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<MarketByPriceUpdate> &event) {
  context_(event, update_market(event));
}

void Strategy::operator()(const Event<OrderAck> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<OrderUpdate> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<TradeUpdate> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<PositionUpdate> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<FundsUpdate> &event) {
  update_market(event);
}

void Strategy::operator()(const Event<RateLimitTrigger> &event) {
  update_market(event);
}



} // mmaker
} // roq