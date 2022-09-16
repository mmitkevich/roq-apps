#pragma once

#include "roq/client.hpp"

#include <roq/client/config.hpp>
#include <roq/client/dispatcher.hpp>

#include "roq/mmaker/order_manager.hpp"
#include "umm/core/context.hpp"
#include "umm/core/type.hpp"
#include "umm/core/type/depth_level.hpp"
#include "umm/core/event.hpp"
#include "umm/core/model.hpp"
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

struct Strategy : client::Handler {

    Strategy(client::Dispatcher& dispatcher, mmaker::Context& context, umm::IQuoter& quoter, mmaker::IOrderManager& order_manager);

    void operator()(const Event<Timer> &) override;
    void operator()(const Event<Connected> &) override;
    void operator()(const Event<Disconnected> &) override;
    void operator()(const Event<DownloadBegin> &) override;
    void operator()(const Event<DownloadEnd> &) override;
    void operator()(const Event<GatewayStatus> &) override;
    void operator()(const Event<ReferenceData> &) override;
    void operator()(const Event<MarketStatus> &) override;
    void operator()(const Event<TopOfBook> &) override;
    void operator()(const Event<MarketByPriceUpdate> &) override;
    void operator()(const Event<OrderAck> &) override;
    void operator()(const Event<OrderUpdate> &) override;
    void operator()(const Event<TradeUpdate> &) override;
    void operator()(const Event<PositionUpdate> &) override;
    void operator()(const Event<FundsUpdate> &) override;
    void operator()(const Event<RateLimitTrigger> &) override;
    void operator()(metrics::Writer &) const override;


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
    cache::Manager cache_;
};



} // mmaker
} // roq