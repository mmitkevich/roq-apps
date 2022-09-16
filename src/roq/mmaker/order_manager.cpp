#include "order_manager.hpp"
#include "umm/core/type.hpp"
#include <roq/order_update.hpp>

namespace roq::mmaker {

void OrderManager::set_dispatcher(client::Dispatcher& dispatcher) {
    this->dispatcher = &dispatcher;
}

void OrderManager::dispatch(TargetOrder const & target) {
    auto market = umm::get_market_ident(target.symbol, target.exchange);
    auto& state = state_[market];
    int64_t index = state.to_price_index(target.price);
    if(target.side==Side::BUY)
        state.bids[index].target_quantity = target.quantity;
    else if(target.side==Side::SELL)
        state.asks[index].target_quantity = target.quantity;
    process();
}

void OrderManager::process() {
    for(auto& [market, state] : state_) {
        process(market, state);
    }
}

bool OrderManager::is_throttled(RequestType req, State& state) {
    return false;
}

bool OrderManager::can_create(TargetOrder const& order, State& state) {
    
    assert(state.orders.find(order.order_id)==std::end(state.orders));

    if(is_throttled(RequestType::CREATE_ORDER, state)) {
        return false;
    }
    if(order.quantity < state.min_trade_vol) {
        return false;
    }
    return true;
}

bool OrderManager::can_cancel(OrderState& order) {
    return order.confirmed.version == order.expected.version;
}

bool OrderManager::can_modify(OrderState& order) {
    return order.confirmed.version == order.expected.version;
}

void OrderManager::process(umm::MarketIdent market, State& state) {
    for(Side side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = (side==Side::BUY) ? state.bids : state.asks;
        for(auto& [price_index, level]: levels) {
            level.expected_quantity = 0;
        }
    }

    for(auto& [order_id, order] : state.orders) {
        auto& levels = (order.side==Side::BUY) ? state.bids : state.asks;
        auto& level = levels[state.to_price_index(order.price)];
        level.price = order.price;
        level.expected_quantity += order.expected.quantity;
    }

    for(auto& [order_id, order] : state.orders) {
        auto& levels = (order.side==Side::BUY) ? state.bids : state.asks;
        auto& level = levels[state.to_price_index(order.price)];
        OrderRef ref {
            .market = market,
            .order_id = order_id,  
            .version = order.confirmed.version 
        };
        if(level.expected_quantity>level.target_quantity) {
            if(can_modify(order)) {
                for(auto& [new_price_index, new_level]: levels) {
                    if(new_level.target_quantity >= new_level.expected_quantity+order.confirmed.quantity) {
                        ///
                        modify_order(ref, TargetOrder {
                            .quantity = order.confirmed.quantity,    // can_modify_qty?new_level.target_quantity-new_level.expected_quantity : order.confirmed.quantity
                            .price = new_level.price,

                        });
                        goto orders_continue;
                    }
                }
            }
            if(can_cancel(order)) {
                cancel_order(ref);
                goto orders_continue;
            }
            // can't do anything with this order (yet)
        }
        orders_continue:;
    }
    for(auto& [order_id, order] : state.orders) {
        auto& levels = (order.side==Side::BUY) ? state.bids : state.asks;
        auto& level = levels[state.to_price_index(order.price)];
        if(level.target_quantity > level.expected_quantity) {
            auto target_order = TargetOrder {
                .exchange = state.exchange,
                .symbol = state.symbol,
                .side = order.side,
                .quantity = level.target_quantity - level.expected_quantity,
                .price = level.price,
            };
            if(can_create(target_order, state)) {
                create_order(target_order);
            }
        }
    }
}

OrderRef OrderManager::create_order(TargetOrder const& target) {
    if(target.order_id>last_order_id) {
        last_order_id = target.order_id;
    } else {
        ++last_order_id;
    }
    OrderRef ref {
        .market = umm::get_market_ident(target.symbol, target.exchange),
        .order_id = last_order_id
    };
    auto& state = state_[ref.market];
    auto& order = state.orders[ref.order_id] = OrderState {
        .order_id = ref.order_id,
        .side = target.side,
        .price = target.price,
        .quantity = target.quantity,
        .traded_quantity = target.quantity,
        .remaining_quantity = target.quantity,
        .sent = {
            .type = RequestType::CREATE_ORDER,
            .status = OrderStatus::UNDEFINED,
            .version = 1,
            .price = target.price,
            .quantity = target.quantity,
            .created_time = {},
        }
    };    
    order.pending = order.sent;
    order.expected = order.sent;

    int source_id = 0;
    dispatcher->send(CreateOrder {
        .account = target.account,
        .order_id = ref.order_id,
        .exchange = target.exchange,
        .symbol = target.symbol,
        //.execution_instructions = target.execution_instructions,
        .order_type = OrderType::LIMIT,
        .quantity = target.quantity,        
        .price = target.price,
    }, source_id);
    return ref;
}

void OrderManager::modify_order(const OrderRef& ref, const TargetOrder & target) {
    auto& state = state_[ref.market];

    auto& order = state.orders[ref.order_id];
    ++order.sent.version;
    order.sent.price = target.price;
    order.sent.quantity = target.quantity;

    order.pending = order.sent;
    order.expected = order.sent;
}

void OrderManager::cancel_order(const OrderRef& ref) {
    auto& state = state_[ref.market];    
    auto& order = state.orders[ref.order_id];
    ++order.sent.version;
    order.sent.type = RequestType::CANCEL_ORDER;
    order.sent.price = order.price;
    order.sent.quantity = 0;

    order.expected = order.sent;
}

OrderRef OrderManager::to_order_ref(const OrderAck& u) const {
    OrderRef ref {
        .market = umm::get_market_ident(u.symbol,u.exchange),
        .order_id = u.order_id,
        .version = u.version
    };
    return ref;
}

OrderRef OrderManager::to_order_ref(const OrderUpdate& u) const {
    OrderRef ref {
        .market = umm::get_market_ident(u.symbol,u.exchange),
        .order_id = u.order_id,
        .version = u.max_response_version
    };
    return ref;
}

void OrderManager::reject_order(const OrderAck& u) {
    auto ref = to_order_ref(u);
    auto& state = state_[ref.market];
    auto& order = state.orders[ref.order_id];
    order.reject_version = u.version;
    order.reject_error = u.error;
    order.reject_reason = u.text;

    order.expected = order.confirmed;
}

void OrderManager::accept_order(const OrderAck& u) {
    auto ref = to_order_ref(u);
    auto& state = state_[ref.market];
    auto& order = state.orders[ref.order_id]; 
    order.accept_version = u.version;
}

void OrderManager::confirm_order(const OrderUpdate& u) {
    auto ref = to_order_ref(u);
    auto& state = state_[ref.market];
    auto& order = state.orders[u.order_id];
    order.confirmed.status = u.status;
    order.confirmed.price = u.price;
    order.confirmed.quantity = u.remaining_quantity;
    order.confirmed.version = u.max_response_version;
}

void OrderManager::complete_order(const OrderUpdate& u) {
    auto ref = to_order_ref(u);
    auto& state = state_[ref.market];
    auto& order = state.orders[u.order_id];
    order.confirmed.status = u.status;
    order.confirmed.price = u.price;
    order.confirmed.quantity = 0;
    order.confirmed.version = u.max_response_version;
    
    order.expected = order.confirmed;
}

void OrderManager::operator()(Event<OrderUpdate> const& event) {
    auto& u = event.value;
    if(u.status==OrderStatus::WORKING) {
        confirm_order(u);
    } else if(u.status == OrderStatus::COMPLETED) {
        confirm_order(u);
        complete_order(u);
    }
}

void OrderManager::operator()(Event<OrderAck> const& event) {
    auto& u = event.value;
    if(u.status == RequestStatus::REJECTED) {
        reject_order(u);
    } else if(u.status == RequestStatus::ACCEPTED) {
        accept_order(u);
    }
}

} // roq::mmaker