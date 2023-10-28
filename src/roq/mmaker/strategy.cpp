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
#include <roq/parameters_update.hpp>
#include <roq/top_of_book.hpp>
#include "roq/pricer/factory.hpp"

namespace roq::mmaker {
//using namespace umm::literals;

void Strategy::initialize() {

  pricer::Factory::initialize_all();

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
      
    core::Quote bid = {
        .price = u.layer.bid_price,
        .volume = u.layer.bid_quantity
    };
    core::Quote ask = {
        .price = u.layer.ask_price,
        .volume = u.layer.ask_quantity
    };

    core::Quotes quotes {
        .market = market_id,
        .symbol = u.symbol,        
        .exchange = u.exchange,        
        .bids = std::span { &bid, 1},
        .asks = std::span { &ask, 1}
    };
    roq::Event event_2 {event.message_info, quotes};  // is it good to keep MessageInfo?
    pricer(event_2);

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
    //umm::Event<umm::DepthUpdate> depth_event;
    //depth_event->market = market;
//      depth_event->bids = context.depth[market].bids;
//      depth_event->asks = context.depth[market].asks;
    //depth_event.set_snapshot(true);
    //depth_event.header.receive_time_utc = event.message_info.receive_time_utc;

    //log::info<2>("DepthUpdate: market {} Depth {}", self()->prn(market), prn(depth_event.value));        
    
    core::Quote bid = {
      .price = !u.bids.empty() ? u.bids[0].price : NAN,
      .volume = !u.bids.empty() ? u.bids[0].quantity : NAN
    };
    core::Quote ask = {
      .price = !u.asks.empty() ? u.asks[0].price : NAN,
      .volume = !u.asks.empty() ? u.asks[0].price : NAN,
    };
    core::Quotes quotes {
      .market = market_id,
      .symbol = u.symbol,      
      .exchange = u.exchange,      
      .bids = std::span { &bid, 1},
      .asks = std::span { &ask, 1}
    };
    roq::Event event_2 {event.message_info, quotes};
    pricer(event_2);

    // publish to udp
    //if(publisher_)
    //    publisher_->dispatch(best_price_event);

}

void Strategy::operator()(const Event<core::ExposureUpdate>& event) {
    // cache position
    auto& u = event.value;
    core.portfolios(event);
    
    // notify pricer
    //umm::Event<umm::PositionUpdate> position_event;
    //position_event.header.receive_time_utc = event.message_info.receive_time_utc;
    //position_event->market = u.market;

    pricer(event);
}



} // namespace roq::mmaker