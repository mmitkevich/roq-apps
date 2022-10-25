#pragma once

#include "roq/client.hpp"

#include "./basic_strategy.hpp"

#include <absl/container/flat_hash_map.h>
#include <roq/cache/gateway.hpp>
#include <roq/client/config.hpp>
#include <roq/client/dispatcher.hpp>

#include "roq/mmaker/order_manager.hpp"
#include "umm/core/context.hpp"
#include "umm/core/type.hpp"
#include "umm/core/type/depth_level.hpp"
#include "umm/core/event.hpp"
#include "umm/core/model_api.hpp"
//#include "umm/core/model.hpp"
//#include "application.hpp"
#include "./context.hpp"
#include "./markets.hpp"


namespace roq {
namespace mmaker {


enum class BestPriceSource {
    UNDEFINED,
    TOP_OF_BOOK,
    MARKET_BY_PRICE,
    VWAP
};

struct Context;

struct Strategy : BasicStrategy<Strategy>, umm::IQuoter::Handler, mmaker::IOrderManager::Handler {
    using Base = BasicStrategy<Strategy>;
    
    using Base::dispatch, Base::self;

    Strategy(client::Dispatcher& dispatcher, mmaker::Context& context, umm::IQuoter& quoter, mmaker::IOrderManager& order_manager);
    virtual ~Strategy();

    /// client::Handler
    void operator()(const Event<ReferenceData> &) override;
    void operator()(const Event<MarketByPriceUpdate> &) override;

    void operator()(const Event<OMSPositionUpdate>& event);

    MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) const {
        return context.get_market_ident(symbol, exchange);
    }

    template<class T>
    auto prn(const T& val) const {
        return context.prn(val);
    }

    template<class T>
    void dispatch(const roq::Event<T> &event) {
        Base::dispatch(event);
        order_manager_(event);
    }

    /// IQuoter::Handler
    void dispatch(const umm::Event<umm::QuotesUpdate> &) override;


private:
    umm::Event<umm::DepthUpdate> get_depth_event(umm::MarketIdent market, const roq::Event<roq::MarketByPriceUpdate>& event);
private:
    mmaker::Context& context;
    mmaker::IOrderManager& order_manager_;
    umm::IQuoter& quoter_;    
    std::vector<roq::MBPUpdate> bids_storage_;
    std::vector<roq::MBPUpdate> asks_storage_;
    std::vector<umm::DepthLevel> depth_bid_update_storage_;
    std::vector<umm::DepthLevel> depth_ask_update_storage_;
    BestPriceSource best_price_source {BestPriceSource::MARKET_BY_PRICE};
    client::Dispatcher& dispatcher_;
    //bool ready_ = false;
};



} // mmaker
} // roq