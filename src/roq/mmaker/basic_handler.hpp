#pragma once

#include "roq/client.hpp"
namespace roq {

template<class Self, class Iface = roq::client::Handler>
struct BasicHandler : Iface {
public:
  Self* self() { return static_cast<Self*>(this); }
  const Self* self() const { return static_cast<const Self*>(this); }

  virtual ~BasicHandler() = default;

  template<class T>
  void dispatch(Event<T> const& event) {}

  // host
  virtual void operator()(Event<Start> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<Stop> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<Timer> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<Connected> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<Disconnected> const & event) { self()->dispatch(event); }

  // control
  virtual void operator()(Event<BatchBegin> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<BatchEnd> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<DownloadBegin> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<DownloadEnd> const & event) { self()->dispatch(event); }

  // config
  virtual void operator()(Event<GatewaySettings> const & event) { self()->dispatch(event); }

  // stream
  virtual void operator()(Event<StreamStatus> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<ExternalLatency> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<RateLimitTrigger> const & event) { self()->dispatch(event); }

  // service
  virtual void operator()(Event<GatewayStatus> const & event) { self()->dispatch(event); }

  // market data
  virtual void operator()(Event<ReferenceData> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<MarketStatus> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<TopOfBook> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<MarketByPriceUpdate> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<MarketByOrderUpdate> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<TradeSummary> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<StatisticsUpdate> const & event) { self()->dispatch(event); }

  // order management
  virtual void operator()(Event<OrderAck> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<OrderUpdate> const & event) { self()->dispatch(event); }
  virtual void operator()(Event<TradeUpdate> const & event) { self()->dispatch(event); }

  // account management
  virtual void operator()(Event<PositionUpdate> const & event) { self()->dispatch(event);  }
  virtual void operator()(Event<FundsUpdate> const & event) { self()->dispatch(event); }

  // broadcast
  virtual void operator()(Event<CustomMetricsUpdate> const & event) { self()->dispatch(event); }

  // ancillary
  virtual void operator()(Event<client::CustomMessage> const & event) { self()->dispatch(event); }

  // parameters
  virtual void operator()(Event<ParameterUpdate> const & event) { self()->dispatch(event); }

  // metrics
  virtual void operator()(metrics::Writer & event) const { }
};

} // namespace roq