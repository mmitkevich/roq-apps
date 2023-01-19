#include "./strategy.hpp"
#include "roq/mmaker/best_price_source.hpp"
#include "roq/mmaker/gateways.hpp"
#include "roq/mmaker/publisher.hpp"
#include "umm/core/type.hpp"
#include "umm/core/event.hpp"
#include "roq/client.hpp"
#include "roq/mmaker/order_manager.hpp"
#include "umm/core/type/depth_array.ipp"
#include <roq/top_of_book.hpp>

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



void Strategy::operator()(const Event<Timer>  & event) {
    Base::operator()(event);  // calls dispatch

    umm::Event<umm::TimeUpdate> timer_event;
    auto recv_time_utc = event.message_info.receive_time_utc;
    timer_event.header.receive_time_utc = recv_time_utc;
    auto now = event.value.now;
    UMM_TRACE("Timer recv_time_utc {} now {}", recv_time_utc, now);
    quoter_->dispatch(timer_event);
}

void Strategy::operator()(const Event<TopOfBook> &event) {
    Base::operator()(event);  // calls dispatch

    const auto& u = event.value;
    MarketIdent market = context.get_market_ident(u.exchange, u.symbol);
    if(context.is_ready(market)) {
      log::info<2>("TopOfBook:  market {} BestPrice {} (from ToB)", context.prn(market), context.prn(context.best_price(market)));
      umm::Event<umm::BestPriceUpdate> best_price_event;
      best_price_event.header.receive_time_utc = event.message_info.receive_time_utc;
      best_price_event->market = market;
      quoter_->dispatch(best_price_event);
      // publish to udp
      if(publisher_)
            publisher_->dispatch(best_price_event);
    }
}

void Strategy::operator()(const Event<MarketByPriceUpdate>& event) {
    Base::operator()(event);

    const auto& u = event.value;
    MarketIdent market = context.get_market_ident(u.symbol, u.exchange);

    if(context.is_ready(market)) {
      umm::Event<umm::DepthUpdate> depth_event;
      depth_event->market = market;
//      depth_event->bids = context.depth[market].bids;
//      depth_event->asks = context.depth[market].asks;
      depth_event.set_snapshot(true);
      depth_event.header.receive_time_utc = event.message_info.receive_time_utc;

      log::info<2>("DepthUpdate: market {} Depth {}", self()->prn(market), prn(depth_event.value));        
      quoter_->dispatch(depth_event);

      umm::Event<umm::BestPriceUpdate> best_price_event;
      auto& best_price = context.best_price[market];
      best_price_event.header.receive_time_utc = event.message_info.receive_time_utc;
      best_price_event->market = market;

      log::info<2>("MarketByPrice:  market {} BestPrice {} (from MBP)", self()->prn(market), self()->prn(best_price));      
      quoter_->dispatch(best_price_event);
      
      // publish to udp
      if(publisher_)
          publisher_->dispatch(best_price_event);
    }
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



void Strategy::operator()(const Event<DownloadBegin> &event) {
  Base::operator()(event);
  umm_mbp_snapshot_sent_.clear();
}

} // namespace mmaker
} // roq