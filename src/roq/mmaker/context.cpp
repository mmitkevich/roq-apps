#include "context.hpp"
#include <roq/client/config.hpp>
#include <roq/support_type.hpp>
#include "umm/core/type/depth_array.ipp"


namespace roq { 
namespace mmaker {


/// Config::Handler
inline void Context::dispatch(roq::client::Config::Handler &handler) const {
    using namespace std::literals;
    //log::info<1>("Config::dispatch"sv);
    markets_.get_markets([&](const auto& data) {
        log::info<1>("symbol={}, exchange={}, market {}"sv, data.symbol, data.exchange, this->markets(data.market));
        handler(client::Symbol {
            .regex = data.symbol,
            .exchange = data.exchange
        });
    });
    handler(client::Settings {
        .order_cancel_policy = OrderCancelPolicy::BY_ACCOUNT
    });
    for(auto& [exchange, account]: accounts_) {
        log::info<1>("account {} exchange {}"sv, account, exchange);
        handler(client::Account {
            .regex = account
        });
    };
}

void Context::initialize(umm::IModel& model) {

    for(auto & [folio, portfolio]: portfolios) {
        for(auto &[market, position]: portfolio) {
            umm::Event<umm::PositionUpdate> event;
            event->market = market;  
            event->portfolio = folio;
            event.header.receive_time_utc = self()->now();
            log::info<1>("portfolio_snapshot folio {}  market {}  position {}", self()->prn(folio), self()->prn(market), position);
            model.dispatch(event);
        }
    }
}

roq::Mask<roq::SupportType> Context::expected_md_support = {};

bool Context::is_ready(umm::MarketIdent market) const {
    bool ready = true;
    ready &= get_market(market, [&](const Markets::Item &market_item) {
        ready &= gateways.get_gateway(market_item.mdata_gateway_id, [&](const Gateways::Item& gateway_item) {
            ready &= gateways.is_ready(expected_md_support, gateway_item.id);
            if(!ready) {
                const roq::cache::Gateway & gateway = gateway_item.base;
                log::info<2>("is_ready=false market {} mdata_gateway_id {} mdata_gateway {} expected {} available {} unavailable {}", 
                    this->prn(market), market_item.mdata_gateway_id, gateway_item.name, 
                    expected_md_support, gateway.state.status.available, gateway.state.status.unavailable);

            }
        });
    });
    ready &= this->tick_rules.tick_size.contains(market) && !std::isnan(this->tick_rules.tick_size[market]);
    return ready;
}

void Context::operator()(const Event<TopOfBook> &event) {
    Base::operator()(event);    
    set_now(event.message_info.receive_time_utc);
    const auto& u = event.value;
    auto [market_cache, is_new] = cache_.get_market_or_create(u.exchange, u.symbol);
    auto market = get_market_ident(market_cache);
    
    umm::BestPrice& best_price = this->best_price[market];
    auto best_price_source = get_best_price_source(market);

    if(best_price_source==BestPriceSource::TOP_OF_BOOK) {
      best_price.bid_price = u.layer.bid_price;
      best_price.ask_price = u.layer.ask_price;
      best_price.bid_volume = u.layer.bid_quantity;
      best_price.ask_volume = u.layer.ask_quantity;
      log::info<2>("Context::MarketByPriceUpdate market {} BestPrice = {} (from ToB)", self()->prn(market), prn(best_price));
    }
}


void Context::operator()(const Event<ReferenceData> &event) {
    auto [market_data, is_new] = cache_.get_market_or_create(event.value.exchange, event.value.symbol);
    bool done = market_data(event);
    auto market = get_market_ident(market_data);
    auto& reference_data = market_data.reference_data;
    assert(!std::isnan(reference_data.tick_size));
    this->tick_rules.min_trade_vol[market] =  reference_data.min_trade_vol;
    this->tick_rules.tick_size[market] =  reference_data.tick_size;

//    const auto mdata_gateway_id = mdata_gateway_id_by_market_[market] = event.message_info.source;
//    log::info<2>("ReferenceData exchange {} symbol {} market {} source {}",
//               event.value.exchange, event.value.symbol, context.prn(market),
//               mdata_gateway_id);
    Base::operator()(event);
}


mmaker::BestPriceSource Context::get_best_price_source(MarketIdent market) const {
  auto best_price_source = BestPriceSource::MARKET_BY_PRICE;
  this->get_market(market, [&](const auto& item) {
      best_price_source = item.best_price_source;
  });
  return best_price_source;
}

void Context::operator()(const Event<MarketByPriceUpdate> &event) {
    set_now(event.message_info.receive_time_utc);
    auto [market_cache, is_new] = cache_.get_market_or_create(event.value.exchange, event.value.symbol);
    auto market = get_market_ident(market_cache);
    auto& mbp = *market_cache.market_by_price;

    bool done = market_cache(event);   

    mbp_depth_.tick_size = mbp.price_increment();

    uint32_t depth_num_levels = markets_.get_or(market, UMM_LAMBDA(_.depth_num_levels), 0u);
    mbp_depth_.num_levels = static_cast<uint16_t>(depth_num_levels);
    if(mbp.max_depth()!=0 && mbp_depth_.num_levels>mbp.max_depth())
        mbp_depth_.num_levels = mbp.max_depth();

    mbp_depth_.update(mbp);


    this->depth[market].tick_size = mbp.price_increment();
    this->depth[market].update(mbp_depth_); 

    log::info<2>("Context::MarketByPriceUpdate market {} max_depth {} depth_num_levels {} MBPDepth = {}", this->prn(market), mbp.max_depth(), depth_num_levels, this->prn(mbp_depth_));

    umm::BestPrice& best_price = this->best_price[market];
    auto best_price_source = get_best_price_source(market);
    if(best_price_source==BestPriceSource::MARKET_BY_PRICE) {
  //      mbp_depth_.update(mbp, 1);  // extract up to 1 level from roq mbp cache
      best_price = mbp_depth_.best_price();
      log::info<2>("Context::MarketByPriceUpdate market {} BestPrice = {} (from MBP)", self()->prn(market), prn(best_price));
    }

    Base::operator()(event);    
}



} // mmaker
} // roq