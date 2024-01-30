// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/exposure.hpp"
#include "roq/core/exposure_update.hpp"
#include "roq/core/hash.hpp"
#include "roq/core/order.hpp"
#include "roq/core/position_source.hpp"
//#include "umm/prologue.hpp"
//#include "umm/core/type.hpp"
#include "roq/core/types.hpp"
#include "roq/core/oms/manager.hpp"
#include <chrono>
#include <roq/buffer_capacity.hpp>
#include <roq/exceptions.hpp>
#include <roq/execution_instruction.hpp>
#include <roq/message_info.hpp>
#include <roq/order_update.hpp>
#include "roq/core/oms/market.hpp"
#include "roq/utils/compare.hpp"
#include <compare>
#include <roq/request_status.hpp>
#include <roq/request_type.hpp>
#include <roq/time_in_force.hpp>
#include <roq/trade.hpp>
#include <roq/trade_update.hpp>
#include "roq/core/clock.hpp"
#include "roq/core/flags/flags.hpp"
#include "roq/logging.hpp"

namespace roq::core::oms {

using namespace std::literals;

Manager::Manager(oms::Handler& handler, client::Dispatcher& dispatcher, core::Manager& core) 
: handler_(handler)
, dispatcher(dispatcher)
, core_{core}
{
    log::debug<2>("oms::Manager this={}", (void*)this);
}


void Manager::operator()(core::TargetQuotes const & target_quotes) {
    log::info<2>("TargetQuotes {}", target_quotes);    
    auto [market,is_new] = emplace_market(target_quotes.symbol, target_quotes.exchange);
    //assert(!is_new);

    assert(!target_quotes.account.empty());

    market.account = target_quotes.account;

    assert(market.exchange == target_quotes.exchange);
    assert(market.symbol == target_quotes.symbol);

    for(auto& [price_index, quote]: market.bids) {
        quote.target_quantity = 0;
        quote.exec_inst = {};
    }
    for(auto& quote: target_quotes.buy) {
        if(!is_empty_value(quote)) {
            auto [level,is_new] = market.emplace_level(Side::BUY, quote.price);
            level.target_quantity = quote.volume;
            level.exec_inst = quote.exec_inst;
        }
    }
    for(auto& [price_index, quote]: market.asks) {
        quote.target_quantity = 0;
        quote.exec_inst = {};
    }
    for(auto& quote: target_quotes.sell) {
        if(!is_empty_value(quote)) {
            auto [level,is_new] = market.emplace_level(Side::SELL, quote.price);
            level.target_quantity = quote.volume;
            level.exec_inst = quote.exec_inst;
        }
    }
    for(auto& [market_id, market] : markets_) {
        process(market);
    }
}


bool Manager::is_throttled(oms::Market& market, RequestType req) {
    return false;
}

bool Manager::can_create(oms::Market& market, const core::TargetOrder & target_order) {
    if(market.pending[target_order.side==Side::SELL]>0)
        return false;

    if(is_throttled(market, roq::RequestType::CREATE_ORDER)) {
        return false;
    }
    if(target_order.quantity < market.min_trade_vol) {
        return false;
    }
    return true;
}

bool Manager::can_cancel(oms::Market& market, oms::Order& order) {
    if(!order.confirmed.version)
        return false;   // still pending
    if(order.is_pending())
        return false;   // something in-flight
    return true;
}

bool Manager::can_modify(oms::Market& market, oms::Order& order) {
    return false;
    //if(!order.is_confirmed())
    if(!order.confirmed.version)    
        return false;   // still pending    
    if(order.is_pending())
        return false;   // something in-flight
    return true;
}

void Manager::process(oms::Market& market) {
    if(market.account.empty())
        return;
    std::chrono::nanoseconds now = this->now();
    auto mask = roq::Mask{roq::SupportType::CREATE_ORDER, roq::SupportType::CANCEL_ORDER};
    bool ready = core_.gateways.is_ready(mask, market.trade_gateway_id, market.account);
    log::info<2>("OMS process now {} symbol {} exchange {} ban {} ready {} tick_size {}",
         now, market.symbol, market.exchange, market.ban_until.count() ? (market.ban_until-now).count()/1E9:NaN, ready, market.tick_size);
    
    if(!ready) {
        return;
    }

    if(std::isnan(market.tick_size)) {
        return;
    }
    
    if(now < market.ban_until) {
        return;
    } else {
        market.ban_until = {};
    }

    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = market.get_levels(side);
        for(auto& [price_index, level]: levels) {
            level.expected_quantity = 0;
            level.confirmed_quantity = 0;
        }
    }

    market.pending = {0,0};
    
    for(auto& [order_id, order] : market.orders) {
        if(order.confirmed.status == OrderStatus::COMPLETED || order.confirmed.status == OrderStatus::CANCELED)
            continue;
        assert(order.confirmed.status!=OrderStatus::COMPLETED);
        assert(order.confirmed.status!=OrderStatus::CANCELED);
        assert(!std::isnan(order.expected.quantity));
        if(order.is_pending())
            market.pending[order.side==Side::SELL]++;

        //assert(utils::compare(order.expected.quantity,0)==std::strong_ordering::greater);
        auto [level,is_new_level] = market.emplace_level(order.side, order.price);
        assert(!std::isnan(level.expected_quantity));          
        level.expected_quantity += order.expected.quantity;
        level.confirmed_quantity += order.confirmed.quantity;
        log::info<2>("OMS order_state order_id={}.{}.{} side={} req={}  price={}  quantity={}"
            " c.status.{} c.price={}  c.quantity={} external_id={}"
            " symbol={} exchange={} market={}",
            order.order_id, order.pending.version, order.confirmed.version,order.side,order.pending.type, order.pending.price,order.pending.quantity,
            order.confirmed.status, order.confirmed.price, order.confirmed.quantity,
            order.external_order_id, market.symbol, market.exchange, market.market);
    }

    log::info<2>("OMS order_state BUY count {} pending {}  SELL count {} pending {}", 
        market.bids.size(), market.pending[0],
        market.asks.size(), market.pending[1]);

    for(Side side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = (side==Side::BUY) ? market.bids : market.asks;
        for(const auto& [price_index, level]: levels) {
            log::info<2>("OMS level_state {} {} target {} expected {} confirmed {}", side, level.price, level.target_quantity, level.expected_quantity, level.confirmed_quantity);
        }
    }
    std::size_t orders_count = market.orders.size();
    for(auto& [order_id, order] : market.orders) {
        auto [level,is_new_level] = market.emplace_level(order.side, order.price);
        if(utils::compare(level.expected_quantity, level.target_quantity)==std::strong_ordering::greater) {
            bool flag = can_modify(market, order);
            log::info<2>("OMS can_modify {} order_id={}.{}.{} side={} req={}  price={}  quantity={}"
            " c.req={}, c.status.{} c.price={}  c.quantity={} external_id={}"
            " symbol={} exchange={} market={}", flag,
            order.order_id, order.pending.version, order.confirmed.version,order.side,order.pending.type, order.pending.price,order.pending.quantity,
            order.confirmed.type, order.confirmed.status, order.confirmed.price, order.confirmed.quantity,
            order.external_order_id, market.symbol, market.exchange, market.market);

            if(flag) {
                const auto& levels = market.get_levels(order.side);                
                for(const auto& [new_price_index, new_level]:  levels) {
                    assert(!std::isnan(new_level.target_quantity));
                    assert(!std::isnan(new_level.expected_quantity));                    
                    assert(!std::isnan(order.confirmed.quantity));
                    if(utils::compare(new_level.target_quantity,new_level.expected_quantity+order.confirmed.quantity)!=std::strong_ordering::less) {
                        ///
                        modify_order(market, order, core::TargetOrder {
                            .quantity = order.confirmed.quantity,    // can_modify_qty?new_level.target_quantity-new_level.expected_quantity : order.confirmed.quantity
                            .price = new_level.price,
                        });
                        goto orders_continue;
                    }
                }
            }
            if(can_cancel(market, order)) {
                this->cancel_order(market, order);
                goto orders_continue;
            }
            // can't do anything with this order (yet)
        }
        orders_continue:;
    }
    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = market.get_levels(side);//(side==Side::BUY) ? bids : asks;
        for(auto& [price_index, level] : levels) {
            assert(!std::isnan(level.target_quantity));
            assert(!std::isnan(level.expected_quantity));
            if(utils::compare(level.target_quantity, level.expected_quantity)==std::strong_ordering::greater) {
                auto target_order = core::TargetOrder {
                    .market = market.market,
                    .side = side,
                    .quantity = level.target_quantity - level.expected_quantity,
                    .price = level.price,
                    .exec_inst = level.exec_inst
                };
                if(can_create(market, target_order)) {
                    //queue_.push_back(target_order); 
                    create_order(market, target_order);
                }
            }
        }
    }
    
    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = market.get_levels(side);

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

oms::Order& Manager::create_order(oms::Market& market, const core::TargetOrder& target) {
    assert(market.market == target.market);
    auto const mask = roq::Mask<roq::SupportType>{roq::SupportType::MODIFY_ORDER};
    assert(core_.gateways.is_ready(mask, market.trade_gateway_id, market.account));
    auto order_id = ++max_order_id;
    assert(market.orders.find(order_id)==std::end(market.orders));
    auto& order = market.orders[order_id] = oms::Order {
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
    auto r = fmt::format_to_n(routing_id.begin(), routing_id.size()-1, "{}.{}",order.order_id, order.pending.version);
    auto routing_id_v = std::string_view { routing_id.data(), r.size };
    roq::Mask<roq::ExecutionInstruction> execution_instructions {};
    
    if(target.exec_inst.has(roq::ExecutionInstruction::PARTICIPATE_DO_NOT_INITIATE))
        execution_instructions.set(roq::ExecutionInstruction::PARTICIPATE_DO_NOT_INITIATE);

    auto create_order = roq::CreateOrder {
        .account = market.account,
        .order_id = order_id,
        .exchange = market.exchange,
        .symbol = market.symbol,
        .side = target.side,
        //.execution_instructions = target.execution_instructions,
        .order_type = OrderType::LIMIT,
        .time_in_force = TimeInForce::GTC,
        .execution_instructions = execution_instructions,                
        .quantity = target.quantity,        
        .price = target.price, 
        .routing_id = routing_id_v
    };
    market_by_order_[order_id] = market.market;
    log::info<1>("OMS create_order {{ order_id={}.{} side={} price={} qty={} symbol={} exchange={} market {}, account={}, execution_instructions={} }}", 
        order_id, order.pending.version, target.side, target.price, target.quantity, 
        market.symbol, market.exchange, market.market, market.account, 
        create_order.execution_instructions);
    dispatcher.send(create_order, source_id);
    return order;
}

void Manager::modify_order(oms::Market& market, oms::Order& order, const core::TargetOrder & target) {
    const auto mask = roq::Mask<roq::SupportType>{roq::SupportType::MODIFY_ORDER};
    assert(core_.gateways.is_ready(mask, market.trade_gateway_id, market.account));
    ++order.pending.version;
    order.pending.type = RequestType::MODIFY_ORDER;
    order.pending.price = target.price;
    order.pending.quantity = target.quantity;

    auto modify_order = ModifyOrder {
        .account = market.account,
        .order_id = order.order_id,
        .quantity = target.quantity,
        .price = target.price,
        .version = order.pending.version,
        .conditional_on_version = order.confirmed.version
    };
    
    log::info<1>("OMS modify_order {{ order_id={}.{}/{}, side={} price={}->{} qty={}->{} external_id={} market={} symbol={} exchange={} account={} }}", 
        modify_order.order_id, modify_order.version, modify_order.conditional_on_version, 
        order.side, order.price, modify_order.price, order.quantity, 
        modify_order.quantity, order.external_order_id, 
        market.market, market.symbol, market.exchange, market.account);
    
    order.expected = order.pending;

    dispatcher.send(modify_order, source_id);
}

void Manager::cancel_order(oms::Market& market, oms::Order& order) {
    const auto mask = roq::Mask<roq::SupportType>{roq::SupportType::CANCEL_ORDER};
    assert(core_.gateways.is_ready(mask, market.trade_gateway_id, market.account));
    assert(market.orders.find(order.order_id)!=std::end(market.orders));
    ++order.pending.version;
    order.pending.type = RequestType::CANCEL_ORDER;
    order.pending.price = order.price;
    order.pending.quantity = 0;

    auto cancel_order = CancelOrder {
        .account = market.account,
        .order_id = order.order_id,
        .version = order.pending.version,
        .conditional_on_version = order.confirmed.version,
    };
    
    log::info<1>("OMS cancel_order {{ order_id={}.{}/{}, side={} price={} qty={} external_id={} market={} symbol={} exchange={} account={} }}", 
        cancel_order.order_id, cancel_order.version, cancel_order.conditional_on_version, 
        order.side, order.price, order.quantity, order.external_order_id, market.market, market.symbol, market.exchange, market.account);
    
    order.expected = order.pending;

    dispatcher.send(cancel_order, source_id);
}


void Manager::order_create_reject(oms::Market& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    assert(u.request_type == RequestType::CREATE_ORDER);
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
    erase_order(market, order.order_id);
}

void Manager::order_modify_reject(oms::Market& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    assert(u.request_type == RequestType::MODIFY_ORDER);
    order.reject_version = u.version;
    order.reject_error = u.error;
    order.reject_reason = u.text;
    order.expected = order.confirmed;
    order.pending.type = RequestType::UNDEFINED;
}

void Manager::order_cancel_reject(oms::Market& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    assert(u.request_type == RequestType::CANCEL_ORDER);
    order.expected = order.confirmed;
    order.pending.type = RequestType::UNDEFINED;    
}

void Manager::order_fwd(oms::Market& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
}

void Manager::order_accept(oms::Market& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    order.accept_version = u.version;
}

void Manager::order_confirm(oms::Market& market, oms::Order& order, const OrderUpdate& u) {
    assert(order.order_id == u.order_id);
    order.external_order_id = u.external_order_id;
    order.confirmed.status = u.order_status;
    order.confirmed.price = u.price;
    order.confirmed.quantity = u.remaining_quantity;
    order.confirmed.version = u.max_accepted_version;
    auto fill_size = u.traded_quantity - order.traded_quantity;
    order.traded_quantity = u.traded_quantity;
    order.pending.type = RequestType::UNDEFINED;
    order.expected = order.confirmed;
    assert(!std::isnan(order.expected.quantity));
    order_fills(market, u, fill_size);
}


bool Manager::reconcile_positions(oms::Market& market) {
    bool is_downloading = core_.gateways.is_downloading(market.trade_gateway_id);
    if(is_downloading)
        return false;
    if(now()-market.last_position_modify_time > std::chrono::seconds{3}) {
        auto delta =  market.position_by_account - market.position_by_orders;    // stable
        if( !std::isnan(delta) && utils::compare(delta, 0.)!=std::strong_ordering::equal) {
            auto prev_by_orders = market.position_by_orders;
            market.position_by_orders = market.position_by_account;
            log::info<1>("OMS reconcile_positions by_orders {}->{} position_by_account {} delta {}",
                prev_by_orders, market.position_by_orders, market.position_by_account, delta);
            
            return true;
        }
    }
    return false;
}

// NOTE: only publishes position in "position_source==ORDERS" mode
void Manager::exposure_update(oms::Market& market) {
    log::info<1>("oms::exposure_update  position_by_orders {} position_by_account {} market {} position_source {}", 
        market.position_by_orders, market.position_by_account, market.market, position_source);        
    
    market.last_position_modify_time = now();

    if(position_source != core::PositionSource::ORDERS)
        return;

    core::Volume position  = market.position_by_orders;

    core::Exposure exposure {
    //            .side = u.side,
    //            .price = u.last_traded_price,
    //            .quantity = fill_size,
        .position_buy = position.max(0),    // FIXME: std::max(NAN, 0) = 0 but empty value should "infect"
        .position_sell = (-position).max(0),
        .market = market.market,        
        .exchange = market.exchange,
        .symbol = market.symbol,
        .portfolio = market.portfolio,             
        .portfolio_name = market.portfolio_name,           
    };    
    core::ExposureUpdate update {
        .exposure = std::span {&exposure, 1}
    };
    handler_(update, *this);
}

template<class T>
void Manager::order_fills(oms::Market& market, const T& u, double fill_size) {
    assert(!std::isnan(fill_size));
    if(utils::compare(fill_size, 0.)==std::strong_ordering::greater) {
        if(u.side==Side::SELL)
            fill_size = -fill_size;
        market.position_by_orders += fill_size;        
        if(position_source==core::PositionSource::ORDERS) {
            exposure_update(market);            
        }
    }
}

//void Manager::operator()(roq::Event<core::ExposureUpdate> const& event) {
//    // FIXME: exposure update comes into oms from outside?
//    handler_(event);
//}

void Manager::order_complete(oms::Market& market, oms::Order& order, const OrderUpdate& u) {
    assert(order.order_id == u.order_id);
    order.confirmed.status = u.order_status;
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
    erase_order(market, order_id);
    order_fills(market, u, fill_size);
}

void Manager::order_canceled(oms::Market& market, oms::Order& order, const OrderUpdate& u) {
    assert(order.order_id == u.order_id);
    order.confirmed.status = u.order_status;
    order.confirmed.type = RequestType::UNDEFINED;
    order.confirmed.price = u.price;
    order.confirmed.quantity = 0;
    order.confirmed.version = u.max_accepted_version;    
    
    order.pending.type = RequestType::UNDEFINED;

    order.expected = order.confirmed;
    assert(!std::isnan(order.expected.quantity));    
    auto order_id = order.order_id;
    erase_order(market, order_id);
}

void Manager::operator()(roq::Event<Timer> const& event) {
    now_ = event.value.now;
    if(now_ - last_process_ >= std::chrono::seconds(1)) {
        for(auto& [market_id, market] : markets_) {
        //    if(!is_downloading(state.gateway_id))
            process(market);
            reconcile_positions(market);
        }
        last_process_ = now_;        
    }
/*    for(auto& [market_id, market] : markets_) {
        //if(!is_downloading(market.gateway_id))
        market.reconcile_positions(market);
    }
*/
}

void Manager::operator()(roq::Event<ReferenceData> const& event) {
    auto& u = event.value;    
    auto [market,is_new] = emplace_market(u.symbol, u.exchange);
    market.tick_size = u.tick_size;
    market.min_trade_vol = u.min_trade_vol;
    Base::operator()(event);
}

void Manager::operator()(roq::Event<GatewayStatus> const& event) {
    log::info<2>("OMS GatewayStatus {}, markets_size {} erase_all_orders_on_gateway_not_ready {}", event, markets_.size(), core::Flags::erase_all_orders_on_gateway_not_ready());
    for(auto& [market_id, market]: markets_) {
        if(!event.value.account.empty() && market.account == event.value.account) {
            if(core::Flags::erase_all_orders_on_gateway_not_ready()) {
                if(!event.value.available.has_all(roq::Mask{roq::SupportType::CREATE_ORDER, roq::SupportType::CANCEL_ORDER})) {
                    log::info<1>("OMS GatewayStatus available {} account {} exchange {} symbol {} market {} erase_all_orders", 
                        event.value.available, market.account, market.exchange, market.symbol, market.market);
                    erase_all_orders(market);
                }
            }
        }
    }
    Base::operator()(event);
}

void Manager::erase_all_orders(oms::Market& market) {
    for(auto& [order_id, order]: market.orders) {
        log::info<1>("OMS erase_all_orders order_id={}.{} side={} price={} remaining_quantity={} status={} symbol={} exchange={} market={}", 
            order_id, order.pending.version, order.side, order.price, order.quantity,
            order.confirmed.status, 
            market.symbol, market.exchange, market.market);    
        market_by_order_.erase(order_id);
    }
    market.orders.clear();
}

bool Manager::erase_order(oms::Market& market, uint64_t order_id) {
    auto iter = market.orders.find(order_id);
    assert(iter!=std::end(market.orders));
    if(iter==std::end(market.orders))
        return false;
    auto& order = iter->second;
    log::info<1>("OMS erase_order order_id={}.{} side={} price={} remaining_quantity={} status={} symbol={} exchange={} market={}", 
        order_id, order.pending.version, order.side, order.price, order.quantity, 
        order.confirmed.status, 
        market.symbol, market.exchange, market.market);
    market.orders.erase(iter);
    market_by_order_.erase(order_id);
    return true;
}

void Manager::operator()(roq::Event<OrderUpdate> const& event) {
    auto& u = event.value;

    auto [market,is_new] = emplace_market(u.symbol, u.exchange);

    auto market_id = core_.markets.get_market_ident(u.symbol, u.exchange);    
    assert(market.market==market.market);

    auto [order,is_new_order] =  market.emplace_order(u.order_id);
    assert(order.order_id == u.order_id);

    log::info<1>("OMS order_update is_new_order={} order_id={}.{} side={} price={} remaining_quantity={} status={} update_type={} symbol={} exchange={} market={}", 
        is_new_order, u.order_id, u.max_accepted_version, u.side, u.price, u.remaining_quantity, u.order_status, u.update_type, u.symbol, u.exchange, market.market);

    if(u.update_type==roq::UpdateType::STALE) {
        erase_order(market, u.order_id);
    } else if(is_new_order) {
        order.side = u.side;
        order.order_id = u.order_id;
        order.price = u.price;
        order.remaining_quantity = u.remaining_quantity;
        order.traded_quantity = u.traded_quantity;
        order.confirmed.type = RequestType::CREATE_ORDER;
        order.confirmed.version = u.max_accepted_version;
        order.confirmed.status = u.order_status;
        order.confirmed.price = u.price;
        order.confirmed.quantity = u.remaining_quantity;
        order.external_order_id = u.external_order_id;
        order.pending.version = u.max_accepted_version;
        order.pending.type = RequestType::UNDEFINED;
        order.expected = order.confirmed;
        market_by_order_[order.order_id] = market.market;
        
        if(u.order_status!=OrderStatus::WORKING) {
            // keep only working
            erase_order(market, order.order_id);
        }
    } else {
        if(u.order_status==OrderStatus::WORKING) {
            order_confirm(market, order, u);
        } else if(u.order_status == OrderStatus::COMPLETED) {
            order_complete(market, order, u);
        } else if(u.order_status == OrderStatus::CANCELED) {
            order_canceled(market, order, u);
        } else if(u.order_status == OrderStatus::REJECTED) {
            erase_order(market, order.order_id);
        }
    }
    process(market);
}

std::pair<oms::Market&, bool> Manager::emplace_market(core::market::Info const& market) {
    auto iter = markets_.find(market.market);
    if(iter!=std::end(markets_)) {
        return {iter->second, false};
    } else {
        assert(market.market);
        assert(!market.exchange.empty());
        assert(!market.symbol.empty());
        oms::Market& market_2 = markets_[market.market];
        market_2.market = market.market;        
        market_2.exchange = market.exchange;
        market_2.symbol = market.symbol;
        market_2.trade_gateway_id = market.trade_gateway_id;
        //market_2.account = market.account;
        //market_2.tick_size = market.tick_size;
        //market_2.min_trade_vol = market.min_trade_vol;
        market_2.last_position_modify_time = now();
        // FIXME: no accounts ?
        
        //if(market_2.account.empty()) {
        //    core_.accounts.get_account(market.exchange, [&](auto& account) {
        //        market_2.account = account;
        //    });
        //}
        log::info<2>("oms::emplace_market symbol {} exchange {} account {} market {}", 
            market_2.symbol, market_2.exchange, market_2.account, market_2.market);
        return {market_2, true};
    }
}


void Manager::operator()(roq::Event<OrderAck> const& event) {
    auto& u = event.value;
    auto iter = market_by_order_.find(u.order_id);
    core::MarketIdent market_id;
    if(iter!=market_by_order_.end())
        market_id = iter->second;
    else {
        log::info<1>("OMS order_ack not_found {} order_id={}.{} side={}, status={} symbol={} exchange={} market={}", 
            u.request_type, u.order_id, u.version, u.side, u.request_status, u.symbol, u.exchange, market_id);
        return;
    }
    auto [market, is_new] = emplace_market(u.symbol, u.exchange);

    if(!market.get_order(u.order_id, [&](oms::Order& order) {
        assert(order.order_id == u.order_id);
        log::info<1>("OMS order_ack {} order_id={}.{} side={}, status={} external_id={}, symbol={} exchange={} market={}, error={}, text={}", 
            u.request_type, u.order_id, u.version, u.side, u.request_status, order.external_order_id, u.symbol, u.exchange, market.market, u.error, u.text);            

        if(u.request_status == RequestStatus::REJECTED) {
            if(u.request_type==RequestType::CANCEL_ORDER) {
                order_cancel_reject(market, order, u);
            } else if(u.request_type==RequestType::CREATE_ORDER) {
                order_create_reject(market, order, u);
            } else if(u.request_type==RequestType::MODIFY_ORDER) {
                order_modify_reject(market, order, u);
            }
            if(u.error==Error::TOO_LATE_TO_MODIFY_OR_CANCEL) {
                order.confirmed.status = OrderStatus::CANCELED;
                order.confirmed.price = order.pending.price;
                order.confirmed.quantity = 0;
                order.confirmed.type = RequestType::UNDEFINED;
            }
            if(u.error==Error::REQUEST_RATE_LIMIT_REACHED || u.error==Error::GATEWAY_NOT_READY) {
                market.ban_until = now_ + reject_timeout_;
            } else {
                process(market);
            }
        } else if(u.request_status == RequestStatus::ACCEPTED) {
            order_accept(market, order, u);
        } else if(u.request_status == RequestStatus::FORWARDED) {
            order_fwd(market, order, u);
        }
    })) {
        log::info<1>("OMS order_ack not_found {} order_id={}.{} side={}, status={} symbol={} exchange={} market={}", 
            u.request_type, u.order_id, u.version, u.side, u.request_status, u.symbol, u.exchange, market.market);            
    }
}


void Manager::operator()(Event<PositionUpdate> const & event) {
    Base::operator()(event);
    auto gateway_id = event.message_info.source;
    bool is_downloading = core_.gateways.is_downloading(gateway_id);    
    auto& u = event.value;    
    auto [market,is_new] = emplace_market(u.symbol, u.exchange);
log::info<2>("PositionUpdate {}", event);
    auto new_position = u.long_quantity - u.short_quantity;
    market.position_by_account = new_position; 
    if(!is_downloading && position_source==core::PositionSource::ACCOUNT) {
        market.position_by_account = new_position;
        log::info<1>("OMS position_update downloading {} account {} position_by_orders {} position_by_account {} symbol {} exchange {} market {}",
            is_downloading, market.account, market.position_by_orders, market.position_by_account,
            market.symbol, market.exchange, market.market);
        //exposure_update(market);
    }
}

/*
double Manager::get_position(std::string_view account, std::string_view symbol, std::string_view exchange) {
    auto iter = position_by_account_.find(account);
    if(iter==std::end(position_by_account_))
        return NAN;
    auto iter_2 = iter->second.find(SymbolExchange { .exchange=exchange, .symbol=symbol });
    if(iter_2==std::end(iter->second))
        return NAN;
    return iter_2->second;
}
*/
void Manager::operator()(Event<FundsUpdate> const & event) {
    Base::operator()(event);
    log::info<2>("FundsUpdate {}", event);
}

void Manager::operator()(roq::Event<DownloadBegin> const& event) {
    log::info<2>("DownloadBegin {}", event);
//    position_by_account_.erase(event.value.account);
//    ready_by_gateway_[event.message_info.source] = false;
    auto& u = event.value;
    if(u.account.empty())
        return;
    for(auto & [market_id, market] : markets_) {
        if(market.account == u.account) {
            if(position_snapshot==core::PositionSnapshot::ACCOUNT) {
                market.position_by_orders = 0;
                market.position_by_account = 0;
            }
            erase_all_orders(market);
            log::info<1>("OMS position_download_begin account {} market.{} {}@{} portfolio.{} {} position_by_orders {} position_by_account {} erase_all_orders",
                market.account, market.market,market.symbol, market.exchange, 
                market.portfolio, market.portfolio_name, 
                market.position_by_orders, market.position_by_account);
        }
    }
}

void Manager::operator()(roq::Event<DownloadEnd> const& event) {
    log::info<2>("DownloadEnd {}", event);
    
    max_order_id  = std::max(max_order_id, event.value.max_order_id);
    auto& u = event.value;
    if(!u.account.empty() && position_snapshot==core::PositionSnapshot::ACCOUNT) {
        for(auto & [market_id, market] : markets_) {
            if(market.account==u.account) {
                market.position_by_orders = market.position_by_account;
                log::info<1>("OMS position_snapshot account {} portfolio.{} {} position_by_orders = position_by_account = {}  market {}",  
                        u.account, market.portfolio, market.portfolio_name, market.position_by_orders, market.market);
                exposure_update(market);
            }
        }
    }
}

void Manager::operator()(roq::Event<RateLimitTrigger> const& event) {
    auto& u = event.value;
    log::info<1>("RateLimitTrigger {}", u);
    switch(u.buffer_capacity) {
       case roq::BufferCapacity::HIGH_WATER_MARK: {
       } break;
       case roq::BufferCapacity::FULL: {
           for(auto &[market_id, market] : markets_ ) {
               if(event.message_info.source_name == market.exchange) {
                auto ban_until = market.ban_until = u.ban_expires;
                log::info<1>("RateLimitTrigger ban_until {} ({}s) exchange {} market {}", 
                    market.ban_until, ban_until.count() ? (ban_until-this->now()).count()/1E9:NaN, market.exchange, market.market);
               }
            }
       } break;
   }
}

std::pair<oms::Market &, bool> Manager::emplace_market(std::string_view symbol, std::string_view exchange) {
   auto [market, is_new] =
       core_.markets.emplace_market({.symbol = symbol, .exchange = exchange});
   return this->emplace_market(market);
}
} // namespace roq::oms