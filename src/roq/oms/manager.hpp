// (c) copyright 2023 Mikhail Mitkevich

#pragma once
#include <absl/container/flat_hash_map.h>
#include <roq/client.hpp>
#include <roq/client/dispatcher.hpp>
#include <roq/client/handler.hpp>
#include <roq/create_order.hpp>
#include <roq/download_end.hpp>
#include <roq/error.hpp>
#include <roq/execution_instruction.hpp>
#include <roq/gateway_status.hpp>
#include <roq/rate_limit_trigger.hpp>
#include <roq/stream_status.hpp>
#include <roq/string_types.hpp>
#include <string>
//#include "roq/core/dispatcher.hpp"
#include "roq/core/handler.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/markets.hpp"
#include "roq/core/clock.hpp"

#include "roq/core/order.hpp"
#include "roq/core/exposure_update.hpp"
#include "roq/core/types.hpp"
#include "roq/core/quote.hpp"
#include "roq/core/quotes.hpp"
#include "roq/core/basic_handler.hpp"
#include "roq/core/position_source.hpp"
#include "roq/oms/order.hpp"
#include "roq/core/exposure_update.hpp"

#include "roq/core/dispatcher.hpp"
#include "roq/oms/handler.hpp"
#include "roq/oms/market.hpp"
#include "roq/core/basic_handler.hpp"

//#include "roq/mmaker/profit_loss.hpp"


namespace roq::oms 
{

struct Manager final 
: core::BasicHandler<Manager, client::Handler>
, core::Dispatcher
{
    using Self = Manager;
    using Base = BasicHandler<Manager, client::Handler>;
    using Handler = oms::Handler;

    using Base::dispatch;
    
public:
    Manager(Manager const&) = delete;
    Manager(Manager &&) = delete;

    Manager(oms::Handler& handler, client::Dispatcher& dispatcher, core::Manager& core);

    std::chrono::nanoseconds now() const { return now_; }

//    bool is_ready(uint32_t source, std::string_view account, roq::Mask<roq::SupportType> mask) const;
//    bool is_downloading(uint32_t source) const;
    
    template<class Config, class Node>
    void configure(const Config& config, Node node);

//    double get_position(std::string_view account, std::string_view symbol, std::string_view exchange);

    std::pair<oms::Market&, bool> emplace_market(core::MarketInfo const& market);

    std::pair<oms::Market &, bool> emplace_market(std::string_view symbol, std::string_view exchange);

    // roq::client::Handler
    void operator()(roq::Event<Timer> const& event) override;
    void operator()(roq::Event<OrderUpdate> const& event) override;
    void operator()(roq::Event<OrderAck> const& event) override;
    void operator()(roq::Event<GatewayStatus> const& event) override;
//    void operator()(roq::Event<Disconnected> const& event);
//    void operator()(roq::Event<Connected> const& event);
//    void operator()(roq::Event<StreamStatus> const& event);
    void operator()(Event<PositionUpdate> const & event) override;
    void operator()(Event<FundsUpdate> const & event) override;
    void operator()(Event<DownloadBegin> const& event) override;
    void operator()(Event<DownloadEnd> const& event) override;
    void operator()(Event<ReferenceData> const& event) override;
    void operator()(Event<RateLimitTrigger> const& event) override;
    
    // roq::pricer::Handler
    void operator()(core::TargetQuotes const& target_quotes) override;    
public:
    core::PositionSource position_source {core::PositionSource::ORDERS};
    core::PositionSnapshot position_snapshot {core::PositionSnapshot::PORTFOLIO};
    core::PortfolioIdent portfolio;

/// IMPLEMENTATION
private:
    bool erase_order(oms::Market& market, uint64_t order_id);
    void erase_all_orders(oms::Market& market);

    void order_create_reject(oms::Market& market, oms::Order& order, const OrderAck& u);
    void order_modify_reject(oms::Market& market, oms::Order& order, const OrderAck& u);
    void order_cancel_reject(oms::Market& market, oms::Order& order, const OrderAck& u);
    void order_fwd(oms::Market& market, oms::Order& order, const OrderAck& u);
    void order_accept(oms::Market& market, oms::Order& order, const OrderAck& u);
    void order_confirm(oms::Market& market, oms::Order& order, const OrderUpdate& u);
    void order_complete(oms::Market& market, oms::Order& order, const OrderUpdate& u);
    void order_canceled(oms::Market& market, oms::Order& order, const OrderUpdate& u);

    template<class T>
    void order_fills(oms::Market& market, const T& u, double fill_size);

    bool is_throttled(oms::Market& market, RequestType req);
    bool can_create(oms::Market& market, const core::TargetOrder & target_order);
    bool can_cancel(oms::Market& market, oms::Order& order);
    bool can_modify(oms::Market& market, oms::Order& order);

    oms::Order& create_order(oms::Market& market, const core::TargetOrder& target);
    void modify_order(oms::Market& market, oms::Order& order, const core::TargetOrder& target);
    void cancel_order(oms::Market& market, oms::Order& order);
    void process(oms::Market& market);
    
    bool reconcile_positions(oms::Market&);
    void exposure_update(oms::Market& market);
private:
    std::chrono::nanoseconds reject_timeout_ = std::chrono::seconds {2};
    uint64_t max_order_id = 0;
    std::array<char, 32> routing_id;
    absl::flat_hash_map<uint64_t, core::MarketIdent> market_by_order_;
    absl::flat_hash_map<core::MarketIdent, Market> markets_;
    absl::flat_hash_map<roq::Exchange, roq::Account> account_by_exchange_;
    int source_id = 0;
    std::chrono::nanoseconds now_{};
    std::chrono::nanoseconds last_process_{};
    
    client::Dispatcher & dispatcher;
    core::Manager& core_;
    oms::Handler& handler_;
    //client::Dispatcher& dispatcher_;
};


template<class Config, class Node>
void oms::Manager::configure(const Config& config, Node node) {
    this->position_snapshot = config.get_value_or(node, "position_snapshot", core::PositionSnapshot::PORTFOLIO);
    this->position_source = config.get_value_or(node, "position_source", core::PositionSource::ORDERS);
    std::string_view portfolio = config.get_string_or(node, "portfolio", {});
    // FIXME:
    // this->portfolio  = portfolio;

    this->reject_timeout_ = std::chrono::nanoseconds { uint64_t(
        std::max(core::Double(100.0), config.get_value_or(node, "reject_timeout_ms", core::Double(1000.0))*1E6)) };
    config.get_nodes(node, "account", [&](auto node) {
        roq::Exchange exchange = config.get_string_or(node, "exchange", {});
        roq::Account account = config.get_string_or(node, "account", {});
        account_by_exchange_[exchange] = account;
    });
}



} // roq::mmaker
/*
template <>
struct fmt::formatter<roq::mmaker::TargetQuotes> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(const roq::mmaker::TargetQuotes &value, Context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        "market {} bid_price {} ask_price {} bid_volume {} ask_volume {}"sv,
        value.market,
        value.bids.size()>0 ? value.bids[0].price.value : NAN,
        value.asks.size()>0 ? value.asks[0].price.value : NAN,
        value.bids.size()>0 ? value.bids[0].volume.value : NAN,
        value.asks.size()>0 ? value.asks[0].volume.value : NAN);
  }
};*/