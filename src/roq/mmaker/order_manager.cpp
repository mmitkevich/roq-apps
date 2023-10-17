// (c) copyright 2023 Mikhail Mitkevich
#include "roq/mmaker/position_source.hpp"
//#include "umm/prologue.hpp"
//#include "umm/core/type.hpp"
#include "./order_manager.hpp"
#include <chrono>
#include <roq/buffer_capacity.hpp>
#include <roq/execution_instruction.hpp>
#include <roq/order_update.hpp>
#include "roq/utils/compare.hpp"
#include <compare>
#include <roq/request_status.hpp>
#include <roq/request_type.hpp>
#include <roq/time_in_force.hpp>
#include <roq/trade.hpp>
#include <roq/trade_update.hpp>
#include "roq/core/clock.hpp"
#include "roq/mmaker/flags/flags.hpp"

namespace roq::mmaker {


void OrderManager::set_dispatcher(client::Dispatcher& dispatcher) {
    this->dispatcher = &dispatcher;
}

inline bool is_empty_quote(const core::Quote& quote) {
    if(umm::is_empty_value(quote.price))
        return true;
    if(umm::is_empty_value(quote.volume))
        return true;
    if(quote.volume==0.)
        return true;
    return false;
}

void OrderManager::dispatch(core::TargetQuotes const & target_quotes) {
    auto market = target_quotes.market;
    auto [state,is_new_state] = get_market_or_create_internal(market);
    assert(!is_new_state);

    state.account = target_quotes.account;
    state.exchange = target_quotes.exchange;
    state.symbol = target_quotes.symbol;

    log::info<2>("TargetQuotes {}", target_quotes);

    for(auto& [price_index, quote]: state.bids) {
        quote.target_quantity = 0;
        quote.exec_inst = {};
    }
    for(auto& quote: target_quotes.bids) {
        if(!is_empty_quote(quote)) {
            auto [level,is_new] = state.get_level_or_create(Side::BUY, quote.price);
            level.target_quantity = quote.volume;
            level.exec_inst = quote.exec_inst;
        }
    }
    for(auto& [price_index, quote]: state.asks) {
        quote.target_quantity = 0;
        quote.exec_inst = {};
    }
    for(auto& quote: target_quotes.asks) {
        if(!is_empty_quote(quote)) {
            auto [level,is_new] = state.get_level_or_create(Side::SELL, quote.price);
            level.target_quantity = quote.volume;
            level.exec_inst = quote.exec_inst;
        }
    }
    for(auto& [market, state] : state_) {
        state.process(*this);
    }
}


bool OrderManager::State::is_throttled(Self& self, RequestType req) {
    return false;
}

bool OrderManager::State::can_create(Self& self, const TargetOrder & target_order) {
    if(pending[target_order.side==Side::SELL]>0)
        return false;

    if(is_throttled(self, roq::RequestType::CREATE_ORDER)) {
        return false;
    }
    if(target_order.quantity < min_trade_vol) {
        return false;
    }
    return true;
}

bool OrderManager::State::can_cancel(Self&self, OrderState& order) {
    if(!order.confirmed.version)
        return false;   // still pending
    if(order.is_pending())
        return false;   // something in-flight
    return true;
}

bool OrderManager::State::can_modify(Self&self, OrderState& order, const TargetOrder* target_order/*=nullptr*/) {
    return false;
    //if(!order.is_confirmed())
    if(!order.confirmed.version)    
        return false;   // still pending    
    if(order.is_pending())
        return false;   // something in-flight
    return true;
}

void OrderManager::State::process(OrderManager& self) {
    if(account.empty())
        return;
    auto now = self.now();
    bool ready = self.is_ready(gateway_id, account, roq::Mask{roq::SupportType::CREATE_ORDER, roq::SupportType::CANCEL_ORDER});
    log::info<2>("OMS process now {} symbol {} exchange {} ban {} ready {} tick_size {}",
         now, symbol, exchange, ban_until.count() ? (ban_until-now).count()/1E9:NaN, ready, tick_size);
    if(!ready) {
        return;
    }
    if(std::isnan(this->tick_size)) {
        return;
    }
    if(now < ban_until) {
        return;
    } else {
        ban_until = {};
    }

    for(Side side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = get_levels(side);
        for(auto& [price_index, level]: levels) {
            level.expected_quantity = 0;
            level.confirmed_quantity = 0;
        }
    }
    pending = {0,0};
    for(auto& [order_id, order] : orders) {
        if(order.confirmed.status == OrderStatus::COMPLETED || order.confirmed.status == OrderStatus::CANCELED)
            continue;
        assert(order.confirmed.status!=OrderStatus::COMPLETED);
        assert(order.confirmed.status!=OrderStatus::CANCELED);
        assert(!std::isnan(order.expected.quantity));
        if(order.is_pending())
            pending[order.side==Side::SELL]++;

        //assert(utils::compare(order.expected.quantity,0)==std::strong_ordering::greater);
        auto [level,is_new_level] = get_level_or_create(order.side, order.price);
        assert(!std::isnan(level.expected_quantity));          
        level.expected_quantity += order.expected.quantity;
        level.confirmed_quantity += order.confirmed.quantity;
        log::info<2>("OMS order_state order_id={}.{}.{} side={} req={}  price={}  quantity={}"
            " c.status.{} c.price={}  c.quantity={} external_id={}"
            " symbol={} exchange={} market={}",
            order.order_id, order.pending.version, order.confirmed.version,order.side,order.pending.type, order.pending.price,order.pending.quantity,
            order.confirmed.status, order.confirmed.price, order.confirmed.quantity,
            order.external_order_id, symbol, exchange, self.context.prn(this->market));
    }

    log::info<2>("OMS order_state BUY count {} pending {}  SELL count {} pending {}", 
        bids.size(), pending[0],
        asks.size(), pending[1]);

    for(Side side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = (side==Side::BUY) ? bids : asks;
        for(const auto& [price_index, level]: levels) {
            log::info<2>("OMS level_state {} {} target {} expected {} confirmed {}", side, level.price, level.target_quantity, level.expected_quantity, level.confirmed_quantity);
        }
    }
    std::size_t orders_count = orders.size();
    for(auto& [order_id, order] : orders) {
        auto [level,is_new_level] = get_level_or_create(order.side, order.price);
        if(utils::compare(level.expected_quantity, level.target_quantity)==std::strong_ordering::greater) {
            bool flag = can_modify(self, order);
            log::info<2>("OMS can_modify {} order_id={}.{}.{} side={} req={}  price={}  quantity={}"
            " c.req={}, c.status.{} c.price={}  c.quantity={} external_id={}"
            " symbol={} exchange={} market={}", flag,
            order.order_id, order.pending.version, order.confirmed.version,order.side,order.pending.type, order.pending.price,order.pending.quantity,
            order.confirmed.type, order.confirmed.status, order.confirmed.price, order.confirmed.quantity,
            order.external_order_id, symbol, exchange, self.context.prn(this->market));

            if(flag) {
                const auto& levels = get_levels(order.side);                
                for(const auto& [new_price_index, new_level]: 
                levels) {
                    assert(!std::isnan(new_level.target_quantity));
                    assert(!std::isnan(new_level.expected_quantity));                    
                    assert(!std::isnan(order.confirmed.quantity));
                    if(utils::compare(new_level.target_quantity,new_level.expected_quantity+order.confirmed.quantity)!=std::strong_ordering::less) {
                        ///
                        modify_order(self, order, TargetOrder {
                            .quantity = order.confirmed.quantity,    // can_modify_qty?new_level.target_quantity-new_level.expected_quantity : order.confirmed.quantity
                            .price = new_level.price,

                        });
                        goto orders_continue;
                    }
                }
            }
            if(can_cancel(self, order)) {
                this->cancel_order(self, order);
                goto orders_continue;
            }
            // can't do anything with this order (yet)
        }
        orders_continue:;
    }
    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = (side==Side::BUY) ? bids : asks;
        for(auto& [price_index, level] : levels) {
            assert(!std::isnan(level.target_quantity));
            assert(!std::isnan(level.expected_quantity));
            if(utils::compare(level.target_quantity, level.expected_quantity)==std::strong_ordering::greater) {
                auto target_order = TargetOrder {
                    .market = market,
                    .side = side,
                    .quantity = level.target_quantity - level.expected_quantity,
                    .price = level.price,
                    .exec_inst = level.exec_inst
                };
                if(can_create(self, target_order)) {
                    //queue_.push_back(target_order); 
                    create_order(self, target_order);
                }
            }
        }
    }
    
    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = get_levels(side);

        for(auto it = levels.begin(); it!=levels.end();) {
            auto& level = it->second;
            assert(!std::isnan(level.price));
            assert(!std::isnan(level.target_quantity));
            assert(!std::isnan(level.expected_quantity));            
            if(roq::utils::compare(level.target_quantity,0.) == std::strong_ordering::equal && 
                roq::utils::compare(level.expected_quantity, 0.) == std::strong_ordering::equal && 
                roq::utils::compare(level.confirmed_quantity, 0.) == std::strong_ordering::equal) {
                auto price = level.price;
                levels.erase(it++);
                log::info<2>("OMS erase_level {} price={} count={}", side, price, levels.size());
            } else {
                it++;
            }
        }
    }
}

OrderManager::OrderState& OrderManager::State::create_order(Self& self, const TargetOrder& target) {
    assert(market == target.market);
    assert(self.is_ready(gateway_id, account, roq::Mask<roq::SupportType>{roq::SupportType::MODIFY_ORDER}));
    auto order_id = ++self.max_order_id;
    assert(orders.find(order_id)==std::end(orders));
    auto& order = orders[order_id] = OrderState {
        .order_id = order_id,
        .side = target.side,
        .price = target.price,
        .quantity = target.quantity,
        .traded_quantity = 0,
        .remaining_quantity = target.quantity,
        .pending = {
            .type = RequestType::CREATE_ORDER,
            .status = OrderStatus::UNDEFINED,
            .version = 1,
            .price = target.price,
            .quantity = target.quantity,
            .created_time = {},
        },
//        .confirmed = {.version = 0},
//        .expected = {.version = 0},
//        .accept_version = 0,
//        .reject_version = 0,
    };    
    order.expected = order.pending;
    assert(order.pending.version==1);
    assert(order.order_id!=0);
    assert(!std::isnan(order.expected.quantity));
    auto r = fmt::format_to_n(self.routing_id.begin(), self.routing_id.size()-1, "{}.{}",order.order_id, order.pending.version);
    auto routing_id_v = std::string_view { self.routing_id.data(), r.size };
    roq::Mask<roq::ExecutionInstruction> execution_instructions {};
    
    if(target.exec_inst.has(roq::ExecutionInstruction::PARTICIPATE_DO_NOT_INITIATE))
        execution_instructions.set(roq::ExecutionInstruction::PARTICIPATE_DO_NOT_INITIATE);

    auto create_order = roq::CreateOrder {
        .account = account,
        .order_id = order_id,
        .exchange = exchange,
        .symbol = symbol,
        .side = target.side,
        //.execution_instructions = target.execution_instructions,
        .order_type = OrderType::LIMIT,
        .time_in_force = TimeInForce::GTC,
        .execution_instructions = execution_instructions,                
        .quantity = target.quantity,        
        .price = target.price, 
        .routing_id = routing_id_v
    };
    self.market_by_order_[order_id] = market;
    log::info<1>("OMS create_order {{ order_id={}.{} side={} price={} qty={} symbol={} exchange={} market {}, account={}, execution_instructions={} }}", 
        order_id, order.pending.version, target.side, target.price, target.quantity, symbol, exchange, self.context.prn(market), account, create_order.execution_instructions);
    self.dispatcher->send(create_order, self.source_id);
    return order;
}

void OrderManager::State::modify_order(Self& self, OrderState& order, const TargetOrder & target) {
    assert(self.is_ready(gateway_id, account, roq::Mask<roq::SupportType>{roq::SupportType::MODIFY_ORDER}));
    ++order.pending.version;
    order.pending.type = RequestType::MODIFY_ORDER;
    order.pending.price = target.price;
    order.pending.quantity = target.quantity;

    auto modify_order = ModifyOrder {
        .account = account,
        .order_id = order.order_id,
        .quantity = target.quantity,
        .price = target.price,
        .version = order.pending.version,
        .conditional_on_version = order.confirmed.version
    };
    
    log::info<1>("OMS modify_order {{ order_id={}.{}/{}, side={} price={}->{} qty={}->{} external_id={} market={} symbol={} exchange={} account={} }}", 
        modify_order.order_id, modify_order.version, modify_order.conditional_on_version, 
        order.side, order.price, modify_order.price, order.quantity, modify_order.quantity, order.external_order_id, self.context.prn(this->market), symbol, this->exchange, this->account);
    
    order.expected = order.pending;

    self.dispatcher->send(modify_order, self.source_id);
}

void OrderManager::State::cancel_order(Self& self, OrderState& order) {
    assert(self.is_ready(gateway_id, account, roq::Mask<roq::SupportType>{roq::SupportType::CANCEL_ORDER}));
    assert(orders.find(order.order_id)!=std::end(orders));
    ++order.pending.version;
    order.pending.type = RequestType::CANCEL_ORDER;
    order.pending.price = order.price;
    order.pending.quantity = 0;

    auto cancel_order = CancelOrder {
        .account = account,
        .order_id = order.order_id,
        .version = order.pending.version,
        .conditional_on_version = order.confirmed.version,
    };
    
    log::info<1>("OMS cancel_order {{ order_id={}.{}/{}, side={} price={} qty={} external_id={} market={} symbol={} exchange={} account={} }}", 
        cancel_order.order_id, cancel_order.version, cancel_order.conditional_on_version, 
        order.side, order.price, order.quantity, order.external_order_id, self.context.prn(this->market), symbol, this->exchange, this->account);
    
    order.expected = order.pending;

    self.dispatcher->send(cancel_order, self.source_id);
}


void OrderManager::State::order_create_reject(Self&self, OrderState& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    assert(u.type == RequestType::CREATE_ORDER);
    order.reject_version = u.version;
    order.reject_error = u.error;
    order.reject_reason = u.text;
    order.confirmed = order.pending;
    order.pending.type = RequestType::UNDEFINED;
    order.confirmed.quantity = 0;
    order.confirmed.status = OrderStatus::REJECTED;

    // nothing is pending
    order.pending.version = order.confirmed.version;

    order.expected = order.confirmed;

    //this->ban_until = std::max(this->ban_until, self.now() + std::chrono::seconds(2));
    erase_order(self, order.order_id);
}

void OrderManager::State::order_modify_reject(Self&self, OrderState& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    assert(u.type == RequestType::MODIFY_ORDER);
    order.reject_version = u.version;
    order.reject_error = u.error;
    order.reject_reason = u.text;
    order.expected = order.confirmed;
    order.pending.type = RequestType::UNDEFINED;
}

void OrderManager::State::order_cancel_reject(Self&self, OrderState& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    assert(u.type == RequestType::CANCEL_ORDER);
    order.expected = order.confirmed;
    order.pending.type = RequestType::UNDEFINED;    
}

void OrderManager::State::order_fwd(Self&self, OrderState& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
}

void OrderManager::State::order_accept(Self&self, OrderState& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    order.accept_version = u.version;
}

void OrderManager::State::order_confirm(Self&self, OrderState& order, const OrderUpdate& u) {
    assert(order.order_id == u.order_id);
    order.external_order_id = u.external_order_id;
    order.confirmed.status = u.status;
    order.confirmed.price = u.price;
    order.confirmed.quantity = u.remaining_quantity;
    order.confirmed.version = u.max_accepted_version;
    auto fill_size = u.traded_quantity - order.traded_quantity;
    order.traded_quantity = u.traded_quantity;
    order.pending.type = RequestType::UNDEFINED;
    order.expected = order.confirmed;
    assert(!std::isnan(order.expected.quantity));
    order_fills(self, u, fill_size);
}

bool OrderManager::State::reconcile_positions(Self& self) {
    bool is_downloading = self.is_downloading(this->gateway_id);
    if(is_downloading)
        return false;
    if(self.now()-last_position_modify_time > std::chrono::seconds{3}) {
        auto delta =  position_by_account - position_by_orders;    // stable
        if( !std::isnan(delta) && utils::compare(delta, 0.)!=std::strong_ordering::equal) {
            auto prev_by_orders = position_by_orders;
            position_by_orders = position_by_account;
            log::info<1>("OMS reconcile_positions by_orders {}->{} position_by_account {} delta {}",
                prev_by_orders, position_by_orders, position_by_account, delta);
            OMSPositionUpdate position_update {
//            .side = u.side,
//            .price = u.last_traded_price,
//            .quantity = fill_size,
                .position = this->position_by_orders,
                .exchange = exchange,
                .symbol = symbol,
                .account = account,   
                .portfolio = self.portfolio,     
                .market = market
            };
            roq::MessageInfo info {};
            roq::Event oms_event(info, position_update);
            self(oms_event);                    
            return true;
        }
    }
    return false;
}

void OrderManager::State::notify_position_update(Self& self) {
    UMM_DEBUG("OMS position_notify position_by_orders {} position_by_account {} market {}", 
        position_by_orders, position_by_account, self.context.prn(market));        

    OMSPositionUpdate position_update {
//        .side = u.side,
//        .price = u.last_traded_price,
//        .quantity = fill_size,
        .position = this->position_by_orders,
        .exchange = exchange,
        .symbol = symbol,
        .account = account,   
        .portfolio = self.portfolio,     
        .market = market
    };

    last_position_modify_time = self.now();

    roq::MessageInfo info {};
    roq::Event oms_event(info, position_update);
    self(oms_event);    
}

template<class T>
void OrderManager::State::order_fills(Self& self, const T& u, double fill_size) {
    assert(!std::isnan(fill_size));
    if(utils::compare(fill_size, 0.)==std::strong_ordering::greater) {
        if(u.side==Side::SELL)
            fill_size = -fill_size;
        position_by_orders += fill_size;
        notify_position_update(self);
    }
}

void OrderManager::operator()(roq::Event<mmaker::OMSPositionUpdate> const& event) {
    if(this->handler_)
        handler_->operator()(event);
}

void OrderManager::set_handler(Handler& handler) {
    handler_ = &handler;
}

void OrderManager::State::order_complete(Self&self, OrderState& order, const OrderUpdate& u) {
    assert(order.order_id == u.order_id);
    order.confirmed.status = u.status;
    order.confirmed.price = u.price;
    order.confirmed.quantity = 0;
    order.confirmed.version = u.max_accepted_version;
    order.confirmed.type = RequestType::UNDEFINED;
    order.pending.type = RequestType::UNDEFINED;
    order.expected = order.confirmed;
    double fill_size = u.traded_quantity - order.traded_quantity;
    order.traded_quantity = u.traded_quantity;
    assert(!std::isnan(order.expected.quantity));
    auto order_id = order.order_id;
    erase_order(self, order_id);
    order_fills(self, u, fill_size);
}

void OrderManager::State::order_canceled(Self&self, OrderState& order, const OrderUpdate& u) {
    assert(order.order_id == u.order_id);
    order.confirmed.status = u.status;
    order.confirmed.type = RequestType::UNDEFINED;
    order.confirmed.price = u.price;
    order.confirmed.quantity = 0;
    order.confirmed.version = u.max_accepted_version;    
    
    order.pending.type = RequestType::UNDEFINED;

    order.expected = order.confirmed;
    assert(!std::isnan(order.expected.quantity));    
    auto order_id = order.order_id;
    erase_order(self, order_id);
}

void OrderManager::operator()(roq::Event<Timer> const& event) {
    now_ = event.value.now;
    if(now_ - last_process_ >= std::chrono::seconds(1)) {
        for(auto& [market, state] : state_) {
        //    if(!is_downloading(state.gateway_id))
            state.process(*this);
            state.reconcile_positions(*this);
        }
        last_process_ = now_;        
    }
/*    for(auto& [market, state] : state_) {
        //if(!is_downloading(state.gateway_id))
        state.reconcile_positions(*this);
    }
*/
}

void OrderManager::operator()(roq::Event<ReferenceData> const& event) {
    auto& u = event.value;
    auto [state,is_new_state] = get_market_or_create(event);
    state.tick_size = u.tick_size;
    state.min_trade_vol = u.min_trade_vol;
    Base::operator()(event);
}

void OrderManager::operator()(roq::Event<GatewayStatus> const& event) {
    log::info<2>("OMS GatewayStatus {}, state.size {} erase_all_orders_on_gateway_not_ready {}", event, state_.size(), mmaker::Flags::erase_all_orders_on_gateway_not_ready());
    for(auto& [market, state]: state_) {
        if(!event.value.account.empty() && state.account == event.value.account) {
            if(mmaker::Flags::erase_all_orders_on_gateway_not_ready()) {
                if(!event.value.available.has_all(roq::Mask{roq::SupportType::CREATE_ORDER, roq::SupportType::CANCEL_ORDER})) {
                    log::info<1>("OMS GatewayStatus available {} account {} exchange {} symbol {} market {} erase_all_orders", 
                        event.value.available, state.account, state.exchange, state.symbol, context.prn(state.market));
                    state.erase_all_orders(*this);
                }
            }
        }
    }
    Base::operator()(event);
}

void OrderManager::State::erase_all_orders(Self& self) {
    for(auto& [order_id, order]: orders) {
        log::info<1>("OMS erase_all_orders order_id={}.{} side={} price={} remaining_quantity={} status={} symbol={} exchange={} market={}", 
            order_id, order.pending.version, order.side, order.price, order.quantity,
            order.confirmed.status, 
            symbol, exchange, self.context.prn(market));    
        self.market_by_order_.erase(order_id);
    }
    orders.clear();
}

bool OrderManager::State::erase_order(Self& self, uint64_t order_id) {
    auto iter = orders.find(order_id);
    assert(iter!=std::end(orders));
    if(iter==std::end(orders))
        return false;
    auto& order = iter->second;
    log::info<1>("OMS erase_order order_id={}.{} side={} price={} remaining_quantity={} status={} symbol={} exchange={} market={}", 
        order_id, order.pending.version, order.side, order.price, order.quantity, 
        order.confirmed.status, 
        symbol, exchange, self.context.prn(market));    
    orders.erase(iter);
    self.market_by_order_.erase(order_id);
    return true;
}

void OrderManager::operator()(roq::Event<OrderUpdate> const& event) {
    auto& u = event.value;
    auto market = context.get_market_ident(u.symbol, u.exchange);
    auto [state,is_new_state] = get_market_or_create(event);
    assert(state.market==market);
    auto [order,is_new_order] =  state.get_order_or_create(u.order_id);
    assert(order.order_id == u.order_id);

    log::info<1>("OMS order_update is_new_order={} order_id={}.{} side={} price={} remaining_quantity={} status={} update_type={} symbol={} exchange={} market={}", 
        is_new_order, u.order_id, u.max_accepted_version, u.side, u.price, u.remaining_quantity, u.status, u.update_type, u.symbol, u.exchange, context.prn(market));

    if(u.update_type==roq::UpdateType::STALE) {
        state.erase_order(*this, u.order_id);
    } else if(is_new_order) {
        order.side = u.side;
        order.order_id = u.order_id;
        order.price = u.price;
        order.remaining_quantity = u.remaining_quantity;
        order.traded_quantity = u.traded_quantity;
        order.confirmed.type = RequestType::CREATE_ORDER;
        order.confirmed.version = u.max_accepted_version;
        order.confirmed.status = u.status;
        order.confirmed.price = u.price;
        order.confirmed.quantity = u.remaining_quantity;
        order.external_order_id = u.external_order_id;
        order.pending.version = u.max_accepted_version;
        order.pending.type = RequestType::UNDEFINED;
        order.expected = order.confirmed;
        market_by_order_[order.order_id] = market;
        
        if(u.status!=OrderStatus::WORKING) {
            // keep only working
            state.erase_order(*this, order.order_id);
        }
    } else {
        if(u.status==OrderStatus::WORKING) {
            state.order_confirm(*this, order, u);
        } else if(u.status == OrderStatus::COMPLETED) {
            state.order_complete(*this, order, u);
        } else if(u.status == OrderStatus::CANCELED) {
            state.order_canceled(*this, order, u);
        } else if(u.status == OrderStatus::REJECTED) {
            state.erase_order(*this, order.order_id);
        }
    }
    state.process(*this);

}

std::pair<OrderManager::OrderState&,bool> OrderManager::State::get_order_or_create(uint64_t order_id) {
    auto iter = orders.find(order_id);
    if(iter==std::end(orders)) {
        auto& order = orders[order_id];
        order.order_id = order_id;
        return {order, true};
    } else {
        return {iter->second, false};
    }
}

std::pair<OrderManager::State&, bool> OrderManager::get_market_or_create_internal(std::string_view symbol, std::string_view exchange) {
    auto market = context.get_market_ident(symbol, exchange);
    auto [state, is_new] = get_market_or_create_internal(market);
    if(is_new) {
        state.symbol = symbol;
        state.exchange = exchange;
        context.get_account(exchange, [&state=state](auto& account) {
            state.account = account;
        });
        log::info<2>("OMS market_create symbol {} exchange {} account {} market {}", 
            state.symbol, state.exchange, state.account, context.prn(market));
    }
    return {state, is_new};
}


std::pair<OrderManager::State&, bool> OrderManager::get_market_or_create_internal(core::MarketIdent market) {
    auto iter = state_.find(market);
    if(iter!=std::end(state_)) {
        return {iter->second, false};
    } else {
        auto& state = state_[market];
        state.last_position_modify_time = now();
        state.market = market;
        log::info<2>("market_create market {}", context.prn(market));
        return {state, true};
    }
}

OrderManager::LevelsMap& OrderManager::State::get_levels(Side side) {
    switch(side){
        case Side::BUY: return bids;
        case Side::SELL: return asks;
        default: assert(false); return *(OrderManager::LevelsMap*)nullptr;
    }
}

std::pair<OrderManager::LevelState&, bool> OrderManager::State::get_level_or_create(Side side, double price) {
    assert(!std::isnan(price));
    auto index = to_price_index(price);
    auto& levels = get_levels(side);
    auto iter = levels.find(index);
    if(iter!=std::end(levels)) {
        assert(utils::compare(iter->second.price, price)==std::strong_ordering::equal);
        return {iter->second, false};
    } else {
        auto & level = levels[index];
        log::info<2>("OMS new_level {} price={} count={}", side, price, levels.size());
        level.price = price;
        return {level, true};
    }
}

void OrderManager::operator()(roq::Event<OrderAck> const& event) {
    auto& u = event.value;
    auto iter = market_by_order_.find(u.order_id);
    core::MarketIdent market;
    if(iter!=market_by_order_.end())
        market = iter->second;
    else {
        log::info<1>("OMS order_ack not_found {} order_id={}.{} side={}, status={} symbol={} exchange={} market={}", 
            u.type, u.order_id, u.version, u.side, u.status, u.symbol, u.exchange, context.prn(market));                    
        return;
    }
    auto [state,is_new_state] = get_market_or_create(event);
//    if(state.symbol.empty()) {
//        state.symbol = u.symbol;
//        state.exchange = u.exchange;
// FIXME:        state.source = event.message_info.source;
//    }
    //state.gateway_id = event.message_info.source;
    if(!state.get_order(u.order_id, [&](auto& order) {
        assert(order.order_id == u.order_id);
        log::info<1>("OMS order_ack {} order_id={}.{} side={}, status={} external_id={}, symbol={} exchange={} market={}, error={}, text={}", 
            u.type, u.order_id, u.version, u.side, u.status, order.external_order_id, u.symbol, u.exchange, context.prn(market), u.error, u.text);            

        if(u.status == RequestStatus::REJECTED) {
            if(u.type==RequestType::CANCEL_ORDER) {
                state.order_cancel_reject(*this, order, u);
            } else if(u.type==RequestType::CREATE_ORDER) {
                state.order_create_reject(*this, order, u);
            } else if(u.type==RequestType::MODIFY_ORDER) {
                state.order_modify_reject(*this, order, u);
            }
            if(u.error==Error::TOO_LATE_TO_MODIFY_OR_CANCEL) {
                order.confirmed.status = OrderStatus::CANCELED;
                order.confirmed.price = order.pending.price;
                order.confirmed.quantity = 0;
                order.confirmed.type = RequestType::UNDEFINED;
            }
            if(u.error==Error::REQUEST_RATE_LIMIT_REACHED || u.error==Error::GATEWAY_NOT_READY) {
                state.ban_until = now_ + reject_timeout_;
            } else {
                state.process(*this);
            }
        } else if(u.status == RequestStatus::ACCEPTED) {
            state.order_accept(*this, order, u);
        } else if(u.status == RequestStatus::FORWARDED) {
            state.order_fwd(*this, order, u);
        }
    })) {
        log::info<1>("OMS order_ack not_found {} order_id={}.{} side={}, status={} symbol={} exchange={} market={}", 
            u.type, u.order_id, u.version, u.side, u.status, u.symbol, u.exchange, context.prn(market));            
    }
}


void OrderManager::operator()(Event<PositionUpdate> const & event) {
    Base::operator()(event);
    bool is_downloading = this->is_downloading(event.message_info.source);    
    auto [state,is_new_state] = get_market_or_create(event);
    auto& u = event.value;
    auto new_position = u.long_quantity - u.short_quantity;
    state.position_by_account = new_position;    
    log::info<2>("PositionUpdate {}", event);
    if(!is_downloading && position_source==PositionSource::ACCOUNT) {
    log::info<1>("OMS position_update downloading {} account {} position_by_orders {} position_by_account {} symbol {} exchange {} market {} portfolio {}",
        is_downloading, state.account, state.position_by_orders, state.position_by_account,
        state.symbol, state.exchange, context.prn(state.market), context.prn(portfolio));
        state.notify_position_update(*this);
    }
}

/*
double OrderManager::get_position(std::string_view account, std::string_view symbol, std::string_view exchange) {
    auto iter = position_by_account_.find(account);
    if(iter==std::end(position_by_account_))
        return NAN;
    auto iter_2 = iter->second.find(SymbolExchange { .exchange=exchange, .symbol=symbol });
    if(iter_2==std::end(iter->second))
        return NAN;
    return iter_2->second;
}
*/
void OrderManager::operator()(Event<FundsUpdate> const & event) {
    Base::operator()(event);
    log::info<2>("FundsUpdate {}", event);
}

void OrderManager::operator()(roq::Event<DownloadBegin> const& event) {
    log::info<2>("DownloadBegin {}", event);
//    position_by_account_.erase(event.value.account);
//    ready_by_gateway_[event.message_info.source] = false;
    auto& u = event.value;
    if(!u.account.empty()) {
        for(auto & [market, state] : state_) {
            if(state.account == u.account) {
                if(position_snapshot==PositionSnapshot::ACCOUNT) {
                    state.position_by_orders = 0;
                    state.position_by_account = 0;
                }
                state.erase_all_orders(*this);
                log::info<1>("OMS position_download_begin account {} symbol {} exchange {} market {} portfolio {} position_by_orders {} position_by_account {} erase_all_orders",
                    state.account, state.symbol, state.exchange, context.prn(state.market), context.prn(portfolio), state.position_by_orders, state.position_by_account);

            }
        }
    }
}

void OrderManager::operator()(roq::Event<DownloadEnd> const& event) {
    log::info<2>("DownloadEnd {}", event);
    
    max_order_id  = std::max(max_order_id, event.value.max_order_id);
    auto& u = event.value;
    
    if(!u.account.empty() && position_snapshot==PositionSnapshot::ACCOUNT) {
        for(auto & [market, state] : state_) {
            if(state.account==u.account) {
                state.position_by_orders = state.position_by_account;
                log::info<1>("OMS position_snapshot account {} portfolio {} position_by_orders = position_by_account = {}  market {}",  
                        u.account, context.prn(portfolio), state.position_by_orders, context.prn(state.market));
                state.notify_position_update(*this);
            }
        }
    }
/*    auto iter = position_by_account_.find(u.account);
    if(iter!=std::end(position_by_account_) && position_snapshot==PositionSnapshot::ACCOUNT) {
        for(auto& [sym, new_position] : iter->second) {
            auto [state, is_new_state] = get_market_or_create(sym.symbol, sym.exchange);
            log::info<1>("OMS position_snapshot account {} portfolio {} snapshot position {} -> {} market {}", 
                    u.account, context.prn(portfolio), state.position_by_orders, new_position, context.prn(state.market));
            state.position_by_orders = new_position;
            OMSPositionUpdate position_update {
        //        .side = u.side,
        //        .price = u.last_traded_price,
        //        .quantity = u.last_traded_quantity,
                .position = state.position_by_orders,
                .exchange = state.exchange,
                .symbol = state.symbol,
                .account = state.account,        
                .portfolio = portfolio,            
                .market = state.market
            };
            roq::MessageInfo info {};
            roq::Event oms_event(info, position_update);
            this->operator()(oms_event);    
        }
    }
*/  
    //ready_by_gateway_[event.message_info.source] = true;
}

void OrderManager::operator()(roq::Event<RateLimitTrigger> const& event) {
    auto& u = event.value;
    log::info<1>("RateLimitTrigger {}", u);
    switch(u.buffer_capacity) {
       case roq::BufferCapacity::HIGH_WATER_MARK: {
       } break;
       case roq::BufferCapacity::FULL: {
           for(auto &[market, state] : state_ ) {
               if(event.message_info.source_name == state.exchange) {
                auto ban_until = state.ban_until = u.ban_expires;
                log::info<1>("RateLimitTrigger ban_until {} ({}s) exchange {} market {}", 
                    state.ban_until, ban_until.count() ? (ban_until-this->now()).count()/1E9:NaN, state.exchange, context.prn(market));
               }
            }
       } break;
   }
}

bool OrderManager::is_ready(uint32_t source, std::string_view account, roq::Mask<roq::SupportType> mask) const {
  return context.gateways.is_ready( mask, source, account);
}

bool OrderManager::is_downloading(uint32_t source) const {
    return context.gateways.is_downloading(source);
}

} // namespace roq::mmaker