#pragma once

#include "roq/client.hpp"

#include <absl/container/flat_hash_map.h>
#include <roq/cache/gateway.hpp>
#include <roq/client/config.hpp>
#include <roq/client/dispatcher.hpp>
#include <roq/top_of_book.hpp>

#include "roq/mmaker/mbp_depth_array.hpp"
#include "roq/mmaker/order_manager.hpp"
#include "roq/mmaker/publisher.hpp"
//#include "umm/core/context.hpp"
//#include "umm/core/type.hpp"
#include "umm/core/type/depth_level.hpp"
//#include "umm/core/event.hpp"
//#include "umm/core/model_api.hpp"
//#include "umm/core/model.hpp"
//#include "application.hpp"
#include "./context.hpp"
#include "./markets.hpp"
#include "roq/quoter/quoter.hpp"

namespace roq {
namespace mmaker {

struct Context;
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

struct Strategy : BasicHandler<Strategy>,  mmaker::IOrderManager::Handler {
    using Base = BasicHandler<Strategy>;
    using Self = Strategy;

    using Base::dispatch, Base::self;

    struct Args {
      mmaker::Context& context;
      std::unique_ptr<roq::quoter::Quoter> quoter;
      std::unique_ptr<mmaker::IOrderManager> order_manager;
      std::unique_ptr<mmaker::Publisher> publisher;
    };

    Strategy(client::Dispatcher& dispatcher, Args args);

    virtual ~Strategy();

    /// client::Handler

    void operator()(const Event<OMSPositionUpdate>& event);

    //MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) const {
    //    return context.get_market_ident(symbol, exchange);
    //}

    template<class T>
    auto prn(const T& val) const {
        return context.prn(val);
    }

    template<class T>
    void dispatch(const roq::Event<T> &event) {
        Base::dispatch(event);
        // notify context
        context(event);
        // notify oms
        order_manager_->operator()(event);
    }

    void operator()(const Event<DownloadBegin> &event);

    void operator()(const Event<TopOfBook> &event);
    void operator()(const Event<MarketByPriceUpdate>& event);
    void operator()(const Event<Timer>  & event);

    /// IQuoter::Handler
    //void dispatch(const umm::Event<umm::QuotesUpdate> &) override;

private:
      umm::Cache<core::MarketIdent, bool> umm_mbp_snapshot_sent_;
private:
    client::Dispatcher& dispatcher_;
    mmaker::Context& context;
    std::unique_ptr<mmaker::IOrderManager> order_manager_;
    //std::unique_ptr<mmaker::IQuoter> quoter_;    
    std::unique_ptr<roq::quoter::Quoter> quoter_;
    std::unique_ptr<mmaker::Publisher> publisher_{};

//    DepthEventFactory depth_event_factory_;
      

};



} // mmaker
} // roq