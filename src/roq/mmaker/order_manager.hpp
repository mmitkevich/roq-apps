#pragma once
#include <absl/container/flat_hash_map.h>
#include <roq/client.hpp>
#include <roq/client/dispatcher.hpp>
#include <roq/create_order.hpp>
#include <roq/error.hpp>
#include <roq/execution_instruction.hpp>
#include <string>
#include "umm/core/type.hpp"
#include "markets.hpp"
#include "clock.hpp"

namespace roq::mmaker 
{


using TargetOrder = roq::CreateOrder;

struct IOrderManager : client::Handler {
    virtual ~IOrderManager() = default;
    virtual void set_dispatcher(client::Dispatcher& dispatcher) = 0;
    virtual void dispatch(TargetOrder const&) = 0;
};

struct OrderRef {
    umm::MarketIdent market {};
    uint32_t order_id = 0;
    uint32_t version = 0;
};

struct OrderManager final : IOrderManager {

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
        bool empty() const { return version == 0; }
        
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

        OrderVersion    sent {};        // last sent version
        uint32_t    accept_version {0};
        uint32_t    reject_version {0};
        roq::Error  reject_error {};
        std::string reject_reason {};
        OrderVersion    confirmed {};   // last version confirmed via OrderUpdate
        OrderVersion    pending {};     // last CreateOrder or ModifyOrder sent
        OrderVersion    expected {};    // expected active version of the order on the exchange in short time
        // CreateOrder => expected = sent = pending = CreatedOrder; confirmed = EmptyOrder
        // ModifyOrder => expected = sent = pending = ModifiedOrder;
        // CancelOrder => expected = sent = CanceledOrder;
        // OrderUpdate => expected = confirmed = pending
        // OrderReject => expected = confirmed
        // OrderCancelReject => expected = confirmed
        // OrderFill   =>  
    };
    struct LevelState {
        double price = NaN;
        double quantity = 0;
        double target_quantity = 0;
        double expected_quantity = 0;
    };
    struct State {
        using LevelsMap = absl::flat_hash_map<int64_t, LevelState>;
        using OrdersMap = absl::flat_hash_map<uint32_t, OrderState>;
        OrdersMap orders;
        LevelsMap bids;
        LevelsMap asks;
        std::string_view exchange;
        std::string_view symbol;
        double tick_size = NAN;
        double min_trade_vol = NAN;
        umm::MarketIdent market {};

        int64_t to_price_index(double price) { return std::roundl(price/tick_size);}
    };
private:
    uint32_t last_order_id = 0;
    absl::flat_hash_map<umm::MarketIdent, State> state_;
public:
    client::Dispatcher *dispatcher = nullptr;
public:
    void process();
    void process(umm::MarketIdent market, State& state);
    void set_dispatcher(client::Dispatcher& dispatcher);
    void dispatch(TargetOrder const& target);
    void operator()(Event<OrderUpdate> const& event);
    void operator()(Event<OrderAck> const& event);
private:

    OrderRef to_order_ref(const OrderAck& u) const;
    OrderRef to_order_ref(const OrderUpdate& u) const;

    bool is_throttled(RequestType req, State& state);
    bool can_create(TargetOrder const& order, State& state);
    bool can_cancel(OrderState& state);
    bool can_modify(OrderState& order);
    OrderRef create_order(TargetOrder const& target);
    void modify_order(const OrderRef& ref, const TargetOrder& target);
    void cancel_order(const OrderRef& ref);
    void reject_order(const OrderAck& u);
    void accept_order(const OrderAck& u);
    void confirm_order(const OrderUpdate& u);
    void complete_order(const OrderUpdate& u);
};

} // roq::mmaker