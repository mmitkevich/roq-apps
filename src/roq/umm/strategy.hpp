#pragma once

#include "roq/client/handler.hpp"
#include "roq/umm/application.hpp"
#include <roq/client/dispatcher.hpp>

namespace roq {
namespace umm {

struct Strategy : client::Handler{

    Strategy(roq::client::Dispatcher&, roq::umm::Application&);

    void operator()(const Event<Timer> &) override;
    void operator()(const Event<Connected> &) override;
    void operator()(const Event<Disconnected> &) override;
    void operator()(const Event<DownloadBegin> &) override;
    void operator()(const Event<DownloadEnd> &) override;
    void operator()(const Event<GatewayStatus> &) override;
    void operator()(const Event<ReferenceData> &) override;
    void operator()(const Event<MarketStatus> &) override;
    void operator()(const Event<MarketByPriceUpdate> &) override;
    void operator()(const Event<OrderAck> &) override;
    void operator()(const Event<OrderUpdate> &) override;
    void operator()(const Event<TradeUpdate> &) override;
    void operator()(const Event<PositionUpdate> &) override;
    void operator()(const Event<FundsUpdate> &) override;
    void operator()(const Event<RateLimitTrigger> &) override;
    void operator()(metrics::Writer &) const override;

    template<class T> void dipatch(const Event<T> &);

};
} // umm
} // roq