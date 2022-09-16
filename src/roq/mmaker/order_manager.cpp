#include "order_manager.hpp"
#include "umm/prologue.hpp"
#include "umm/core/type.hpp"
#include <roq/order_update.hpp>
#include "roq/utils/compare.hpp"
#include <compare>
#include <roq/time_in_force.hpp>

namespace roq::mmaker {

void OrderManager::set_dispatcher(client::Dispatcher& dispatcher) {
    this->dispatcher = &dispatcher;
}

inline bool is_empty_quote(const Quote& quote) {
    if(umm::is_empty_value(quote.price))
        return true;
    if(umm::is_empty_value(quote.volume))
        return true;
    if(quote.volume==0.)
        return true;
    return false;
}

void OrderManager::dispatch(TargetQuotes const & target_quotes) {
    auto market = target_quotes.market;
    auto& state = state_[market];
    state.account = target_quotes.account;
    state.exchange = target_quotes.exchange;
    state.symbol = target_quotes.symbol;
    for(auto& [price_index, quote]: state.bids) {
        quote.target_quantity = 0;
    }
    for(auto& quote: target_quotes.bids) {
        if(!is_empty_quote(quote)) {
            int64_t index = state.to_price_index(quote.price);
            auto & level = state.bids[index];
            level.price = quote.price;
            level.target_quantity = quote.volume;
        }
    }
    for(auto& [price_index, quote]: state.asks) {
        quote.target_quantity = 0;
    }
    for(auto& quote: target_quotes.bids) {
        if(!is_empty_quote(quote)) {
            int64_t index = state.to_price_index(quote.price);
            auto & level = state.bids[index];
            level.price = quote.price;
            level.target_quantity = quote.volume;
        }
    }
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

    if(is_throttled(RequestType::CREATE_ORDER, state)) {
        return false;
    }
    if(order.quantity < state.min_trade_vol) {
        return false;
    }
    return true;
}

bool OrderManager::can_cancel(OrderState& order) {
    if(!order.confirmed.version)
        return false;   // still pending
    if(order.confirmed.version != order.sent.version)
        return false;   // something in-flight
    return true;
}

bool OrderManager::can_modify(OrderState& order) {
    return false;
    if(!order.confirmed.version)
        return false;   // still pending    
    if(order.confirmed.version != order.sent.version)
        return false;   // something in-flight
    return true;
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
        assert(!std::isnan(level.expected_quantity));          
        assert(!std::isnan(order.expected.quantity));
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
                    assert(!std::isnan(new_level.target_quantity));
                    assert(!std::isnan(new_level.expected_quantity));                    
                    assert(!std::isnan(order.confirmed.quantity));
                    if(utils::compare(new_level.target_quantity,new_level.expected_quantity+order.confirmed.quantity)!=std::strong_ordering::less) {
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
    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = (side==Side::BUY) ? state.bids : state.asks;
        for(auto& [price_index, level] : levels) {
            assert(!std::isnan(level.target_quantity));
            assert(!std::isnan(level.expected_quantity));
            if(utils::compare(level.target_quantity, level.expected_quantity)==std::strong_ordering::greater) {
                auto target_order = TargetOrder {
                    .market = market,
                    .side = side,
                    .quantity = level.target_quantity - level.expected_quantity,
                    .price = level.price,
                };
                if(can_create(target_order, state)) {
                    create_order(target_order);
                }
            }
        }
    }
    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = (side==Side::BUY) ? state.bids : state.asks;

        for(auto it = levels.begin(); it!=levels.end(); it++) {
            auto& level = it->second;
            assert(!std::isnan(level.target_quantity));
            assert(!std::isnan(level.expected_quantity));            
            if(roq::utils::compare(level.target_quantity,0.) == std::strong_ordering::equal && 
                roq::utils::compare(level.expected_quantity, 0.) == std::strong_ordering::equal ) {
                levels.erase(it);
            }
        }
    }
}

OrderRef OrderManager::create_order(TargetOrder const& target) {
    OrderRef ref {
        .market = target.market,
        .order_id = ++max_order_id
    };
    assert(state_.find(ref.market)!=std::end(state_));
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
    assert(!std::isnan(order.expected.quantity));
    int source_id = 0;
    dispatcher->send(CreateOrder {
        .account = state.account,
        .order_id = ref.order_id,
        .exchange = state.exchange,
        .symbol = state.symbol,
        .side = target.side,
        //.execution_instructions = target.execution_instructions,
        .order_type = OrderType::LIMIT,
        .time_in_force = TimeInForce::GTC,
        .quantity = target.quantity,        
        .price = target.price,
        //.routing_id = "mmaker"
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

void OrderManager::order_reject(const OrderAck& u) {
    auto ref = to_order_ref(u);
    auto& state = state_[ref.market];
    auto& order = state.orders[ref.order_id];
    order.reject_version = u.version;
    order.reject_error = u.error;
    order.reject_reason = u.text;

    order.expected = order.confirmed;
}

void OrderManager::order_accept(const OrderAck& u) {
    auto ref = to_order_ref(u);
    auto& state = state_[ref.market];
    auto& order = state.orders[ref.order_id]; 
    order.accept_version = u.version;
}

void OrderManager::order_confirm(const OrderUpdate& u) {
    auto ref = to_order_ref(u);
    auto& state = state_[ref.market];
    auto& order = state.orders[u.order_id];
    order.confirmed.status = u.status;
    order.confirmed.price = u.price;
    order.confirmed.quantity = u.remaining_quantity;
    order.confirmed.version = u.max_response_version;
    
    order.expected = order.confirmed;
    assert(!std::isnan(order.expected.quantity));
}

void OrderManager::order_complete(const OrderUpdate& u) {
    auto ref = to_order_ref(u);
    auto& state = state_[ref.market];
    auto& order = state.orders[u.order_id];
    order.confirmed.status = u.status;
    order.confirmed.price = u.price;
    order.confirmed.quantity = 0;
    order.confirmed.version = u.max_response_version;
    
    order.expected = order.confirmed;
    assert(!std::isnan(order.expected.quantity));
}

void OrderManager::order_canceled(const OrderUpdate& u) {
    auto ref = to_order_ref(u);   
    auto& state = state_[ref.market];
    auto& order = state.orders[u.order_id];
    order.confirmed.status = u.status;
    order.confirmed.price = u.price;
    order.confirmed.quantity = 0;
    order.confirmed.version = u.max_response_version;    

    order.expected = order.confirmed;
    assert(!std::isnan(order.expected.quantity));    
}

void OrderManager::operator()(Event<OrderUpdate> const& event) {
    auto& u = event.value;
    if(u.status==OrderStatus::WORKING) {
        order_confirm(u);
    } else if(u.status == OrderStatus::COMPLETED) {
        //order_confirm(u);
        order_complete(u);
    } else if(u.status == OrderStatus::CANCELED) {
        order_canceled(u);
    }
}

void OrderManager::operator()(Event<OrderAck> const& event) {
    auto& u = event.value;
    if(u.status == RequestStatus::REJECTED) {
        order_reject(u);
    } else if(u.status == RequestStatus::ACCEPTED) {
        order_accept(u);
    }
}

void OrderManager::operator()(Event<DownloadEnd> const& event) {
    max_order_id  = std::max(max_order_id, event.value.max_order_id);
}

} // roq::mmaker