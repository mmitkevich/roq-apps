#include "./strategy.hpp"
#include "roq/mmaker/publisher.hpp"
#include "umm/core/type.hpp"
#include "umm/core/event.hpp"
#include "roq/client.hpp"
#include "roq/mmaker/order_manager.hpp"
#include "umm/core/type/depth_array.ipp"

namespace roq {
namespace mmaker {
using namespace umm::literals;

Strategy::Strategy(client::Dispatcher& dispatcher, mmaker::Context& context, 
  std::unique_ptr<umm::IQuoter> quoter, std::unique_ptr<mmaker::IOrderManager> order_manager, std::unique_ptr<mmaker::Publisher> publisher)
: dispatcher_(dispatcher)
, context( context )
, quoter_(std::move(quoter))
, order_manager_(std::move(order_manager))
, publisher_(std::move(publisher))
{
    if(order_manager_) {
      order_manager_->set_dispatcher(dispatcher_);
      order_manager_->set_handler(*this);
    }
    if(publisher_) {
      publisher_->set_dispatcher(dispatcher_);
    }
    if(quoter_) {
      quoter_->set_handler(*this);
    }    
}

Strategy::~Strategy() {}

void Strategy::operator()(const Event<ReferenceData> &event) {
    auto [market_data, is_new] = cache_.get_market_or_create(event.value.exchange, event.value.symbol);
    bool done = market_data(event);
    auto market = context.get_market_ident(market_data);
    auto& reference_data = market_data.reference_data;
    assert(!std::isnan(reference_data.tick_size));
    context.tick_rules.min_trade_vol[market] =  reference_data.min_trade_vol;
    context.tick_rules.tick_size[market] =  reference_data.tick_size;
    Base::operator()(event);
}



bool Strategy::is_ready(umm::MarketIdent market) const {
  if(!Base::is_ready(market))
    return false;
  if(!context.tick_rules.tick_size.contains(market) || std::isnan(context.tick_rules.tick_size[market]))
    return false;
  return true;
}

void Strategy::operator()(const Event<MarketByPriceUpdate> &event) {
    auto [market_cache, is_new] = cache_.get_market_or_create(event.value.exchange, event.value.symbol);
    auto market = context.get_market_ident(market_cache);
    auto& mbp = *market_cache.market_by_price;

    bool done = market_cache(event);   

    mbp_depth_.update(mbp);
    mbp_depth_.tick_size = mbp.price_increment();
    mbp_depth_.num_levels = std::min(mbp.max_depth(), uint16_t{1000});
    context.depth[market].tick_size = mbp.price_increment();

    umm::BestPrice& best_price = context.best_price[market];
    if(best_price_source==BestPriceSource::MARKET_BY_PRICE) {
  //      mbp_depth_.update(mbp, 1);  // extract up to 1 level from roq mbp cache
        best_price = mbp_depth_.best_price();
    }

  log::info<2>("Strateg::MarketByPriceUpdate market {} BestPrice = {}, MBPDepth = {}", self()->prn(market), prn(best_price), prn(mbp_depth_));

    if(is_ready(market)) {
      umm::Event<umm::DepthUpdate> depth_event;
      if(!umm_mbp_snapshot_sent_(market, false)) {  // FIXME: this required due to possibly absent tick_size at time of MBP
        umm_mbp_snapshot_sent_[market] = true;
//        mbp_depth_.update(mbp, 0);  // extracts all levels from roq mbp cache
        depth_event = depth_event_factory_(market,  mbp_depth_.bids, mbp_depth_.asks);
        //context.depth[market].handle(depth_event);        
        
        context.depth[market].update(mbp_depth_); // TODO: recompute our dense depth from platform-specific depth

        depth_event.set_snapshot(true);
        depth_event.header.receive_time_utc = event.message_info.receive_time_utc;
        log::info<2>("MBPSnapshot market {} {}", self()->prn(market), prn(depth_event.value));        
      } else {
        depth_event = depth_event_factory_(market, event.value.bids, event.value.asks);
        //context.depth[market].handle(depth_event);
        
        context.depth[market].update(mbp_depth_); // TODO: recompute our dense depth from platform-specific depth

        depth_event.set_snapshot(false);
        depth_event.header.receive_time_utc = event.message_info.receive_time_utc;
        log::info<2>("MBPUpdate:  market {}  {}", self()->prn(market), prn(depth_event.value));        
      }
      //FIXME store to the cache: this->depth[market].
      quoter_->dispatch(depth_event);

      umm::Event<umm::QuotesUpdate> quotes_event;
      quotes_event.header.receive_time_utc = event.message_info.receive_time_utc;
      quotes_event->market = market;
      quoter_->dispatch(quotes_event);
      
      // publish to udp
      if(publisher_)
          publisher_->dispatch(quotes_event);
    }
    Base::operator()(event);    
}

void Strategy::operator()(const Event<Timer>  & event) {
    umm::Event<umm::TimeUpdate> timer_event;
    auto now = timer_event.header.receive_time_utc = event.message_info.receive_time_utc;
    context.set_now(now);
    quoter_->dispatch(timer_event);
    Base::operator()(event);
}

void Strategy::dispatch(const umm::Event<umm::QuotesUpdate> &event) {
    umm::MarketIdent market = event->market;
    context.get_market(market, [&](const auto& data) {
      context.get_account(data.exchange, [&](std::string_view account) {
        umm::QuotesView quotes = quoter_->get_quotes(market);
        TargetQuotes target_quotes {
          .market = market,
          .account = account,
          .exchange = data.exchange,
          .symbol = data.symbol,
          .portfolio = "FIXME",       
          .bids = quotes.bids,
          .asks = quotes.asks,
        };
        if(order_manager_)
          order_manager_->dispatch(target_quotes);
      });
    });
}

void Strategy::operator()(const Event<OMSPositionUpdate>& event) {
    // cache position
    auto& u = event.value;
    log::info<2>("Strategy::OMSPositionUpdate position {} portfolio {} market {}", 
      u.position, context.prn(u.portfolio), context.prn(u.market));
    context.portfolios[u.portfolio][u.market] = u.position;
    
    // notify quoter
    umm::Event<umm::PositionUpdate> position_event;
    position_event.header.receive_time_utc = event.message_info.receive_time_utc;
    position_event->market = u.market;
    quoter_->dispatch(position_event);
}

} // mmaker
} // roq