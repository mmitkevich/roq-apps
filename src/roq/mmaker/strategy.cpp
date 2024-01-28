// (c) copyright 2023 Mikhail Mitkevich
#include "roq/mmaker/strategy.hpp"
#include "roq/core/best_price_source.hpp"
#include "roq/core/gateways.hpp"
//#include "roq/mmaker/publisher.hpp"
//#include "umm/core/type.hpp"
//#include "umm/core/event.hpp"
#include "roq/client.hpp"
#include "roq/core/exposure_update.hpp"
#include "roq/oms/manager.hpp"
//#include "umm/core/type/depth_array.ipp"
#include <roq/cache/market_by_price.hpp>
#include <roq/parameters_update.hpp>
#include <roq/top_of_book.hpp>
#include "roq/dag/factory.hpp"

#include "roq/mmaker/application.hpp"

#define USE_LQS
#define USE_PRICER

#ifdef USE_LQS
#include "roq/lqs/pricer.hpp"
#endif

#ifdef USE_DAG
#include "roq/dag/pricer.hpp"
#endif

namespace roq::mmaker {
using namespace std::literals;
  
std::unique_ptr<core::Handler> Strategy::make_pricer() {
  #ifdef USE_LQS
  if(strategy_name == "lqs"sv) {
    return std::make_unique<lqs::Pricer>(oms, core);
  } 
  #endif
  #ifdef USE_DAG
  if(strategy_name == "dag"sv) {
    dag::Factory::initialize_all();
    return std::make_unique<dag::Pricer>(oms, core);
  }
  #endif
  throw roq::RuntimeError("pricer '{}' was not found"sv, strategy_name);
  return {};
}

Strategy::Strategy(client::Dispatcher& dispatcher, Application& context)
: dispatcher_(dispatcher)
, strategy_name(context.strategy_name)
, config(context.config)
, core()
, oms(*this, dispatcher, core)
{
  pricer = make_pricer();
  config.configure(*this);
}

Strategy::~Strategy() {}

void Strategy::operator()(const Event<TopOfBook> &event) {
    Base::operator()(event);  // calls dispatch

    const auto& u = event.value;
    core::MarketIdent market_id = core.get_market_ident(u.symbol, u.exchange);
    if(!core.is_ready(market_id))
      return;
    //log::info<2>("TopOfBook:  market {} BestPrice {} (from ToB)", context.prn(market), context.prn(context.best_price(market)));
    
      //umm::Event<umm::BestPriceUpdate> best_price_event;
      //best_price_event.header.receive_time_utc = event.message_info.receive_time_utc;
      //best_price_event->market = market;
      
    core::ExecQuote buy = {
        .price = u.layer.bid_price,
        .volume = u.layer.bid_quantity
    };

    core::ExecQuote sell = {
        .price = u.layer.ask_price,
        .volume = u.layer.ask_quantity
    };

    core::Quotes quotes {
        .market = market_id,
        .symbol = u.symbol,        
        .exchange = u.exchange,        
        .buy = std::span { &buy, 1},
        .sell = std::span { &sell, 1},
    };

    roq::Event event_2 {event.message_info, quotes};  // is it good to keep MessageInfo?
    (*pricer)(event_2);

      // publish to udp
      //if(publisher_)
      //      publisher_->dispatch(best_price_event);
}

void Strategy::operator()(const Event<MarketByPriceUpdate>& event) {
    Base::operator()(event);

    const auto& u = event.value;
    core::MarketIdent market_id = core.get_market_ident(u.symbol, u.exchange);

    if(!core.is_ready(market_id))
      return;

      // use roq internal caching to extract BBO from MBP
    auto [market_cache, is_new] = core.cache.get_market_or_create(u.exchange, u.symbol);
    bool done = market_cache(event);   
    cache::MarketByPrice& mbp = *market_cache.market_by_price;
    mbp.extract_2(layers_, 1);
    
    core::ExecQuote buy, sell;
    
    if(!layers_.empty()) {
      buy.price = layers_[0].bid_price;
      buy.volume = layers_[0].bid_quantity;
      sell.price = layers_[0].ask_price;
      sell.volume = layers_[0].ask_quantity;
    }

    core::Quotes quotes {
      .market = market_id,
      .symbol = u.symbol,      
      .exchange = u.exchange,      
      .buy = std::span { &buy, 1},
      .sell = std::span { &sell, 1}
    };

    roq::Event event_2 {event.message_info, quotes};
    (*pricer)(event_2);

    // publish to udp
    //if(publisher_)
    //    publisher_->dispatch(best_price_event);
}

// provide REST interface to push volume to liquidate ? 

// CustomMetrics [client_strategy_id=999 = {BTC-PER=1000, BTC_FUT=-1000}] message from roq-risk-manager
void Strategy::operator()(const Event<core::ExposureUpdate>& event) {
    // cache position
    auto& u = event.value;
    core.portfolios(event);
    
    // notify pricer
    //umm::Event<umm::PositionUpdate> position_event;
    //position_event.header.receive_time_utc = event.message_info.receive_time_utc;
    //position_event->market = u.market;

    (*pricer)(event);
}



} // namespace roq::mmaker