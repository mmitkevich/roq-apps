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
#include "umm/printable.hpp"
#include "roq/mmaker/context.hpp"
#include "markets.hpp"
#include "clock.hpp"

#include "umm/core/type.hpp"
#include "umm/core/type/quote.hpp"
#include "roq/mmaker/basic_handler.hpp"
#include "roq/mmaker/position_source.hpp"
#include "roq/mmaker/gateways.hpp"

namespace roq::mmaker 
{

// TODO struct Quote { double price; double quantity;  uint64_t flags; }
using Quote = umm::Quote;

struct TargetOrder {
    MarketIdent market {};
    Side side {Side::UNDEFINED};
    double quantity {NaN};    
    double price {NaN};
    uint64_t flags = 0;
};

struct TargetQuotes : umm::BasicPrintable<TargetQuotes> {
    MarketIdent market {};
    std::string_view account {};
    std::string_view exchange {};
    std::string_view symbol {};
    std::string_view portfolio {};
    std::span<const Quote> bids = {};
    std::span<const Quote> asks = {};

    template<class Format, class Context>
    constexpr decltype(auto) format(Format& fmt, const Context& context) const {
        using namespace std::literals;
        return fmt::format_to(
        fmt.out(),
        "market {} bid_price {} ask_price {} bid_volume {} ask_volume {}"sv,
        umm::prn(market, context),
        bids.size()>0 ? bids[0].price.value : NAN,
        asks.size()>0 ? asks[0].price.value : NAN,
        bids.size()>0 ? bids[0].volume.value : NAN,
        asks.size()>0 ? asks[0].volume.value : NAN);
    }
};

struct IOrderManager : client::Handler {
    struct Handler;
    virtual ~IOrderManager() = default;
    virtual void set_dispatcher(client::Dispatcher& dispatcher) = 0;
    virtual void set_handler(Handler& handler) = 0;
    virtual void dispatch(TargetQuotes const &quotes) = 0;
};


struct OMSPositionUpdate {
    Side side {Side::UNDEFINED};
    double price {NAN};
    double quantity {NAN};
    double position {NAN};
    std::string_view exchange;
    std::string_view symbol;
    std::string_view account;
    umm::PortfolioIdent portfolio;
    MarketIdent market;
};

struct IOrderManager::Handler {
    virtual ~Handler() = default;
    virtual void operator()(const roq::Event<OMSPositionUpdate>& event) {}
};

struct OrderManager final : BasicHandler<OrderManager, IOrderManager>
{
    using Self = OrderManager;
    using Base = BasicHandler<OrderManager, IOrderManager>;

    using Base::dispatch;
    
    struct OrderVersion  {
        RequestType type {RequestType::UNDEFINED};   // CREATE, MODIFY, CANCEL
        OrderStatus status {OrderStatus::UNDEFINED};
        uint32_t version = 0;
        double price = NaN;
        double quantity = NaN;
        std::chrono::nanoseconds created_time {};
        void clear() {
            type = RequestType::UNDEFINED;
            price = NaN;
            quantity = 0;
            status = OrderStatus::UNDEFINED;
            version = 0;
            created_time = {};
        }
        
        std::chrono::nanoseconds latency() const { 
            return Clock::now() - created_time;
        }
    };

    struct OrderState {
        uint32_t order_id = 0;
        Side side = Side::UNDEFINED;
        double price = NaN;
        double quantity = NaN;
        double traded_quantity = NaN;
        double remaining_quantity = NaN;

        bool is_pending() const { return pending.type!=RequestType::UNDEFINED ; }
        bool is_confirmed() const { return confirmed.type!=RequestType::UNDEFINED; }

        OrderVersion    pending {};     // pending order state (create/cancel/modify) and associated request type
        OrderVersion    confirmed {};   // cofirmed (via OrderUpdate) order state and request type succeeded
        OrderVersion    expected {};    // expected order state, which could be either pending or confirmed, including request type
        uint32_t    accept_version {0};
        uint32_t    reject_version {0};
        roq::Error  reject_error {roq::Error::UNDEFINED};
        std::string reject_reason {};

        roq::ExternalOrderId external_order_id;
    };

    struct LevelState {
        double price = NaN;
        double quantity = 0;
        double target_quantity = 0;
        double expected_quantity = 0;
        double confirmed_quantity = 0;
        uint64_t flags = 0;
    };
    using LevelsMap = absl::flat_hash_map<int64_t, LevelState>;
    using OrdersMap = absl::flat_hash_map<uint32_t, OrderState>;
    
    struct State {
        OrdersMap orders;
        LevelsMap bids;
        LevelsMap asks;
        roq::Exchange exchange;
        uint8_t gateway_id = (uint8_t)-1;
        roq::Symbol symbol;
        roq::Account account;
        double tick_size = NAN;
        double min_trade_vol = NAN;
        umm::MarketIdent market {};
        std::chrono::nanoseconds ban_until {};
        std::array<uint16_t,2> pending={0,0};
        double position_by_orders = 0;
        double position_by_account = 0;
        std::chrono::nanoseconds last_position_modify_time;
      public:
        bool reconcile_positions(Self& self);

        int64_t to_price_index(double price) { 
            assert(!std::isnan(tick_size));
            return std::roundl(price/tick_size);
        }
        void notify_position_update(Self&self);
        std::pair<LevelState&, bool> get_level_or_create(Side side, double price);        
        LevelsMap& get_levels(Side side);
        // order&, is_new
        std::pair<OrderState&,bool> get_order_or_create(uint32_t order_id);
        template<class Fn>
        bool get_order(uint32_t order_id, Fn&& fn);
        bool erase_order(Self& self, uint32_t order_id);

        void order_create_reject(Self& self, OrderState& order, const OrderAck& u);
        void order_modify_reject(Self& self, OrderState& order, const OrderAck& u);
        void order_cancel_reject(Self& self, OrderState& order, const OrderAck& u);
        void order_fwd(Self& self, OrderState& order, const OrderAck& u);
        void order_accept(Self& self, OrderState& order, const OrderAck& u);
        void order_confirm(Self& self, OrderState& order, const OrderUpdate& u);
        void order_complete(Self& self, OrderState& order, const OrderUpdate& u);
        void order_canceled(Self& self, OrderState& order, const OrderUpdate& u);

        template<class T>
        void order_fills(Self& self, const T& u, double fill_size);

        bool is_throttled(Self& self, RequestType req);
        bool can_create(Self& self, const TargetOrder & target_order);
        bool can_cancel(Self& self, OrderState& order);
        bool can_modify(Self& self, OrderState& order, const TargetOrder* target_order=nullptr);

        OrderState& create_order(Self& self, const TargetOrder& target);
        void modify_order(Self& self, OrderState& order, const TargetOrder& target);
        void cancel_order(Self& self, OrderState& order);
        void process(Self& self);
    };
public:
    PositionSource position_source {PositionSource::ORDERS};
    PositionSnapshot position_snapshot {PositionSnapshot::PORTFOLIO};
    umm::PortfolioIdent portfolio;
private:
    std::chrono::nanoseconds reject_timeout_ = std::chrono::seconds {2};
    uint32_t max_order_id = 0;
    Handler* handler_ {nullptr};
    std::array<char, 32> routing_id;
    absl::flat_hash_map<uint32_t, umm::MarketIdent> market_by_order_;
    absl::flat_hash_map<MarketIdent, State> state_;
    client::Dispatcher *dispatcher = nullptr;
    mmaker::Context& context;
    std::deque<TargetOrder> queue_;
    int source_id = 0;
    std::chrono::nanoseconds now_{};
    std::chrono::nanoseconds last_process_{};

    //absl::flat_hash_map<uint8_t, GatewayFlags> ready_by_gateway_;
    Gateways& gateways_;

    //absl::flat_hash_map<Account, absl::flat_hash_map<SymbolExchange, double>> position_by_account_;
public:
    OrderManager(mmaker::Context& context, mmaker::Gateways& gateway_manager) 
    : context(context)
    , gateways_(gateway_manager)
    {}

    std::chrono::nanoseconds now() const { return now_; }
   /* bool is_ready(uint8_t gateway_id) const {
        auto iter = ready_by_gateway_.find(gateway_id);
        if(iter==std::end(ready_by_gateway_)) {
            return false;
        }
        return iter->second;
    }*/

    bool is_ready(uint32_t source, std::string_view account, roq::Mask<roq::SupportType> mask) const;
    bool is_downloading(uint32_t source) const;
    
    template<class Config, class Node>
    void configure(const Config& config, Node node);

//    double get_position(std::string_view account, std::string_view symbol, std::string_view exchange);

    std::pair<State&, bool> get_market_or_create_internal(MarketIdent market);
    std::pair<State&, bool> get_market_or_create_internal(std::string_view symbol, std::string_view exchange);
    template<class T>
    std::pair<State&, bool> get_market_or_create(const roq::Event<T>& event);
    void set_handler(Handler& handler);
    void set_dispatcher(client::Dispatcher& dispatcher);
    void dispatch(TargetQuotes const& target_quotes);
    void operator()(roq::Event<Timer> const& event);
    void operator()(roq::Event<OrderUpdate> const& event);
    void operator()(roq::Event<OrderAck> const& event);
//    void operator()(roq::Event<GatewayStatus> const& event);
//    void operator()(roq::Event<Disconnected> const& event);
//    void operator()(roq::Event<Connected> const& event);
//    void operator()(roq::Event<StreamStatus> const& event);
    void operator()(Event<PositionUpdate> const & event);
    void operator()(Event<FundsUpdate> const & event);
    void operator()(roq::Event<DownloadBegin> const& event);
    void operator()(roq::Event<DownloadEnd> const& event);
    void operator()(roq::Event<ReferenceData> const& event);
    void operator()(roq::Event<RateLimitTrigger> const& event);
    void operator()(roq::Event<OMSPositionUpdate> const& event);

};


template<class Fn>
inline bool OrderManager::State::get_order(uint32_t order_id, Fn&& fn) {
    auto iter = orders.find(order_id);
    if(iter!=orders.end()) {
        fn(iter->second);
        return true;
    }
    return false;
}

template<class Config, class Node>
void OrderManager::configure(const Config& config, Node node) {
    this->position_snapshot = config.get_value_or(node, "position_snapshot", PositionSnapshot::PORTFOLIO);
    this->position_source = config.get_value_or(node, "position_source", PositionSource::ORDERS);
    this->portfolio = config.get_value_or(node, "portfolio", umm::PortfolioIdent{});
    this->reject_timeout_ = std::chrono::nanoseconds { uint64_t(
        std::max(umm::Double(100.0), config.get_value_or(node, "reject_timeout_ms", umm::Double(1000.0))*1E6)) };
}


template<class T>
std::pair<OrderManager::State&, bool> OrderManager::get_market_or_create(const Event<T>& event) {
    auto [state, is_new] = get_market_or_create_internal(event.value.symbol, event.value.exchange);
    state.gateway_id = event.message_info.source;
    return {state, is_new};
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