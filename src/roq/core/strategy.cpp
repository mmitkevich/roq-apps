// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/strategy.hpp"
#include "roq/core/best_quotes_source.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/gateway/manager.hpp"
#include "roq/client.hpp"
#include "roq/core/exposure_update.hpp"
#include "roq/core/market.hpp"
#include "roq/core/types.hpp"
#include "roq/core/oms/manager.hpp"
#include <roq/cache/market_by_price.hpp>
#include <roq/market_by_price_update.hpp>
#include <roq/parameters_update.hpp>
#include <roq/top_of_book.hpp>


#include "roq/core/application.hpp"

namespace roq::core {

Strategy::Strategy(client::Dispatcher& dispatcher, core::config::Manager& config, core::Factory factory, std::string strategy_name)
: dispatcher_(dispatcher)
, config(config)
, factory(factory)
, strategy_name(strategy_name)
, core()
, oms(*this, dispatcher, core)
{
  pricer = factory(*this);
  config.configure(*this);
  core.portfolios.set_handler(this);
}

Strategy::~Strategy() {}

void Strategy::operator()(const Event<TopOfBook> &event) {
    Base::operator()(event);  // calls dispatch

    const auto& u = event.value;
    core::MarketIdent market = core.get_market_ident(u.symbol, u.exchange);
    
    if(!core.is_ready(market))
      return;
    
    core.markets.get_market(market, [&](const core::market::Info& minfo) {
      if(minfo.best_quotes_source==core::BestQuotesSource::TOP_OF_BOOK) {
          auto [best_quotes,is_new] = core.best_quotes.emplace_quotes(market); 
          
          best_quotes.buy.price = u.layer.bid_price;
          best_quotes.buy.volume = u.layer.bid_quantity;
          best_quotes.sell.price = u.layer.ask_price;
          best_quotes.buy.volume = u.layer.ask_quantity;

          core::Quotes quotes {
              .market = market,
              .symbol = u.symbol,        
              .exchange = u.exchange,        
              .buy = std::span { &best_quotes.buy, 1},
              .sell = std::span { &best_quotes.sell, 1},
          };
          roq::Event event_2 {event.message_info, quotes};  // is it good to keep MessageInfo?
          this->operator()(event_2);          
      }
    });
    

    // publish to udp
    //if(publisher_)
    //      publisher_->dispatch(best_price_event);
}

void Strategy::operator()(const Event<MarketByPriceUpdate>& event) {
    Base::operator()(event);

    const auto& u = event.value;
    core::MarketIdent market = core.get_market_ident(u.symbol, u.exchange);

    if(!core.is_ready(market))
      return;

    core.markets.get_market(market, [&](core::market::Info const &minfo) {
      if(minfo.best_quotes_source == core::BestQuotesSource::MARKET_BY_PRICE) {
          // use roq internal caching to extract BBO from MBP
          auto [market_cache, is_new] = core.cache.get_market_or_create(u.exchange, u.symbol);
          bool done = market_cache(event);   
          cache::MarketByPrice& mbp = *market_cache.market_by_price;
          mbp.extract_2(layers_, 1);

          // cache best_quotes (FIXME)
          auto [best_quotes, is_new_quotes] = core.best_quotes.emplace_quotes(market); 
          
          if(!layers_.empty()) {
            best_quotes.buy.price = layers_[0].bid_price;
            best_quotes.buy.volume = layers_[0].bid_quantity;
            best_quotes.sell.price = layers_[0].ask_price;
            best_quotes.sell.volume = layers_[0].ask_quantity;
          } else {
            best_quotes.clear();
          }

          core::Quotes quotes {
              .market = market,
              .symbol = u.symbol,        
              .exchange = u.exchange,        
              .buy = std::span { &best_quotes.buy, 1},
              .sell = std::span { &best_quotes.sell, 1},
          };
          
          roq::Event event_2 {event.message_info, quotes};
          this->operator()(event_2);
      }
    });

    // publish to udp
    //if(publisher_)
    //    publisher_->dispatch(best_price_event);
}

void Strategy::operator()(const Event<core::Quotes>& event) {
    (*pricer)(event);
}

void Strategy::operator()(core::Trade const & trade) {
  // route trade from oms to portfolios
  core.portfolios(trade);
}

// route portfolios notification into pricer
void Strategy::operator()(core::ExposureUpdate const& u) {
    
    MessageInfo info;
    roq::Event event {info, u};
    
    (*pricer)(event);
}

} // namespace roq::mmaker