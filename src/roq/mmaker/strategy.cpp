#include "./strategy.hpp"
#include "roq/client.hpp"
#include "roq/mmaker/order_manager.hpp"

namespace roq {
namespace mmaker {
using namespace umm::literals;

Strategy::Strategy(client::Dispatcher& dispatcher, mmaker::Context& context, umm::IQuoter& quoter, mmaker::IOrderManager& order_manager)
: dispatcher_(dispatcher)
, context( context )
, quoter_(quoter)
, cache_(client::MarketByPriceFactory::create)
, order_manager_(order_manager)
{
    order_manager.set_dispatcher(dispatcher);
    umm::IQuoter::Handler& h = *this;
    quoter_.set_handler(h);
}


void Strategy::operator()(const Event<Timer> &event) {
  
}

void Strategy::operator()(const Event<Connected> &event) {
  
}

void Strategy::operator()(const Event<Disconnected> &event) {
  
}

void Strategy::operator()(const Event<DownloadBegin> &event) {
    ready_ = false;  
}

void Strategy::operator()(const Event<DownloadEnd> &event) {
    order_manager_(event);
    ready_ = true;
}

void Strategy::operator()(const Event<GatewayStatus> &event) {
    order_manager_(event);
}

void Strategy::operator()(const Event<ReferenceData> &event) {
    auto [market_data, is_new] = cache_.get_market_or_create(event.value.exchange, event.value.symbol);
    bool done = market_data(event);
    auto market = context.get_market_ident(market_data);
    auto& reference_data = market_data.reference_data;
    context.tick_rules.min_trade_vol[market] =  reference_data.min_trade_vol;
    context.tick_rules.tick_size[market] =  reference_data.tick_size;
    order_manager_(event);
}

void Strategy::operator()(const Event<MarketStatus> &event) {
  
}

void  Strategy::operator()(const Event<TopOfBook> &) {

}

umm::Event<umm::DepthUpdate> Strategy::get_depth_event(umm::MarketIdent market, const Event<MarketByPriceUpdate>& event) {
    depth_bid_update_storage_.resize(event.value.bids.size());
    for(std::size_t i=0;i<event.value.bids.size();i++) {
      depth_bid_update_storage_[i].price = event.value.bids[i].price;
      depth_bid_update_storage_[i].volume = event.value.bids[i].quantity;
    }
    depth_ask_update_storage_.resize(event.value.asks.size());
    for(std::size_t i=0;i<event.value.asks.size();i++) {
      depth_ask_update_storage_[i].price = event.value.asks[i].price;
      depth_ask_update_storage_[i].volume = event.value.asks[i].quantity;
    }        
    umm::Event<umm::DepthUpdate> umm_event;
    umm_event->market = market;
    umm_event->bids = depth_bid_update_storage_;
    umm_event->asks = depth_ask_update_storage_;
    return umm_event;
}

void Strategy::operator()(const Event<MarketByPriceUpdate> &event) {
    auto [market_data, is_new] = cache_.get_market_or_create(event.value.exchange, event.value.symbol);
    bool done = market_data(event);
    auto market = context.get_market_ident(market_data);
    auto& mbp = *market_data.market_by_price;
    const auto [bids_size, asks_size] = mbp.size();
    bids_storage_.resize(bids_size);
    asks_storage_.resize(asks_size);
    auto [bids, asks] = mbp.extract(bids_storage_, asks_storage_);
    
    if(best_price_source==BestPriceSource::MARKET_BY_PRICE) {
        umm::BestPrice& best_price = context.best_price[market];
        if(bids.size()>0) {
          best_price.bid_price = bids[0].price;
          best_price.bid_volume = bids[0].quantity;
        }
        if(asks.size()>0) {
          best_price.ask_price = asks[0].price;
          best_price.ask_volume = asks[0].quantity;
        }
    }
    if(!ready_)
      return;
    umm::Event<umm::DepthUpdate> depth_event = get_depth_event(market, event);
    //FIXME store to the cache: this->depth[market].
    quoter_.dispatch(depth_event);

    umm::Event<umm::QuotesUpdate> quotes_event;
    quotes_event->market = market;
    quoter_.dispatch(quotes_event);
}

void Strategy::dispatch(const umm::Event<umm::QuotesUpdate> &event) {
    umm::MarketIdent market = event->market;
    context.get_market(market, [&](Markets::Item const& item) {
      context.get_account(item.exchange, [&](std::string_view account) {
        umm::QuotesView quotes = quoter_.get_quotes(market);
        TargetQuotes target_quotes {
          .market = market,
          .account = account,
          .exchange = item.exchange,
          .symbol = item.symbol,
          .portfolio = "FIXME",       
          .bids = quotes.bids,
          .asks = quotes.asks,
        };
        order_manager_.dispatch(target_quotes);
      });
    });
}

void Strategy::operator()(const Event<OrderAck> &event) {
   if(!ready_)
      return;
    order_manager_(event);
}

void Strategy::operator()(const Event<OrderUpdate> &event) {
   if(!ready_)
      return;
    order_manager_(event);
}

void Strategy::operator()(const Event<TradeUpdate> &event) {

}

void Strategy::operator()(const Event<PositionUpdate> &event) {

}

void Strategy::operator()(const Event<FundsUpdate> &event) {
}

void Strategy::operator()(const Event<RateLimitTrigger> &event) {

}

void Strategy::operator()(metrics::Writer &) const {

}

} // mmaker
} // roq