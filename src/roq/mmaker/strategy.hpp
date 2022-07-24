#pragma once

#include "roq/client/handler.hpp"
#include <roq/cache/manager.hpp>
#include <roq/client/dispatcher.hpp>
#include "context.hpp"
#include "application.hpp"


#include "umm/quoter.hpp"

namespace roq {
namespace mmaker {

struct Strategy : client::Handler {

    Strategy(client::Dispatcher& dispatcher, Context& context);


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

    template<class T> 
    cache::Market& update_market(const Event<T> &event);
private:
    Context& context_;
    std::unique_ptr<umm::Quoter> quoter_;
    client::Dispatcher& dispatcher_;
    cache::Manager cache_;
};

} // mmaker
} // roq