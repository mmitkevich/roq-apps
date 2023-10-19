#pragma once

#include "roq/config/toml_config.hpp"
#include "roq/core/manager.hpp"
#include "roq/client.hpp"
#include "roq/core/basic_handler.hpp"
#include <absl/container/flat_hash_map.h>
#include <roq/cache/gateway.hpp>
#include <roq/client/config.hpp>
#include <roq/client/dispatcher.hpp>
#include <roq/top_of_book.hpp>
#include <type_traits>

//#include "roq/mmaker/mbp_depth_array.hpp"
#include "roq/oms/manager.hpp"
//#include "roq/mmaker/publisher.hpp"
//#include "umm/core/context.hpp"
//#include "umm/core/type.hpp"
//#include "umm/core/type/depth_level.hpp"
//#include "umm/core/event.hpp"
//#include "umm/core/model_api.hpp"
//#include "umm/core/model.hpp"
//#include "application.hpp"
//#include "./context.hpp"
#include "roq/core/markets.hpp"
#include "roq/pricer/manager.hpp"

namespace roq {
namespace mmaker {

/*
struct DepthEventFactory {
    std::vector<umm::DepthLevel> bids;
    std::vector<umm::DepthLevel> asks;

    template<class 
    umm::Event<umm::DepthUpdate> operator()(core::MarketIdent market, std::span<roq::MBPUpdate> bids, std::span<roq::MBPUpdate> asks);
};

inline umm::Event<umm::DepthUpdate> DepthEventFactory::operator()(core::MarketIdent market, std::span<MBPUpdate> bids, std::span<MBPUpdate> asks) {
    this->bids.resize(bids.size());
    for(std::size_t i=0;i<bids.size();i++) {
      double qty = bids[i].quantity;
      if(qty==0)
        qty = NAN;
      this->bids[i] = umm::DepthLevel {
        .price = bids[i].price,
        .volume = qty
      };
      if(this->bids[i].volume.value==0) {
          this->bids[i].volume = {};
      }
    }
    this->asks.resize(asks.size());
    for(std::size_t i=0;i<asks.size();i++) {
      double qty = asks[i].quantity;
      if(qty==0)
        qty = NAN;

      this->asks[i] = umm::DepthLevel {
        .price = asks[i].price,
        .volume = qty
      };
    }        
    umm::Event<umm::DepthUpdate> event;
    event->market = market;
    event->bids = this->bids;
    event->asks = this->asks;
    return event;
}
*/


struct Strategy : core::BasicHandler<Strategy>,  oms::Handler {
    using Base = core::BasicHandler<Strategy>;
    using Self = Strategy;

    using Base::dispatch, Base::self;

    Strategy(Strategy const&) = delete;
    Strategy(Strategy&&) = delete;

    template<class Context>
    Strategy(client::Dispatcher& dispatcher, Context& context)
    : dispatcher_(dispatcher)
    , oms(*this, dispatcher, core)
    , pricer(oms, core)
    , config(context.config)
    , strategy(context.strategy) {}
    
    ~Strategy();

    /// client::Handler

    void operator()(const Event<core::ExposureUpdate>& event);

    //MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) const {
    //    return context.get_market_ident(symbol, exchange);
    //}

    //template<class T>
    //auto prn(const T& val) const {
    //    return context.prn(val);
    //}


    // route events
    template<class T>
    void dispatch(const roq::Event<T> &event) {
        Base::dispatch(event);
        if constexpr(std::is_invocable_v<decltype(oms), decltype(event)>) {
          oms(event);
        }
        if constexpr(std::is_invocable_v<decltype(core), decltype(event)>) {
          core(event);
        }
        if constexpr(std::is_invocable_v<decltype(pricer), decltype(event)>) {
          pricer(event);        
        }
    }

    void operator()(const Event<DownloadBegin> &event);

    void operator()(const Event<TopOfBook> &event);
    void operator()(const Event<MarketByPriceUpdate>& event);
    void operator()(const Event<Timer>  & event);

    /// IQuoter::Handler
    //void dispatch(const umm::Event<umm::QuotesUpdate> &) override;

private:
    //core::Hash<core::MarketIdent, bool> umm_mbp_snapshot_sent_;
private:
    client::Dispatcher& dispatcher_;

    // components
    core::Manager core;
    oms::Manager oms;
    pricer::Manager pricer;
    config::TomlConfig& config;
    std::string strategy;
};



} // mmaker
} // roq