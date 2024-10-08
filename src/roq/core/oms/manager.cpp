// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/exposure.hpp"
#include "roq/core/exposure_update.hpp"
#include "roq/core/hash.hpp"
#include "roq/core/order.hpp"
#include "roq/core/position_source.hpp"
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
#include "roq/core/string_utils.hpp"

namespace roq::core::oms {

using namespace std::literals;

Manager::Manager(oms::Handler& handler, client::Dispatcher& dispatcher, core::Manager& core) 
: handler_(handler)
, dispatcher(dispatcher)
, core{core}
{
    log::debug<2>("oms::Manager this={}", (void*)this);
}


void Manager::operator()(Event<ParametersUpdate> const& event) {
    
    for(roq::Parameter const& p: event.value.parameters) {
        auto [prefix, label] = core::split_prefix(p.label, ':');
        if(prefix != "oms"sv)
            continue;
        auto key = oms::Market { 
            .market = core.markets.get_market_ident(p.symbol, p.exchange),
            .symbol = p.symbol,
            .exchange = p.exchange,            
            .account = p.account,
            .portfolio = p.strategy_id, // FIXME: portfolio_id == strategy_id
            .strategy = p.strategy_id,
        };
        auto [book, is_new] = emplace_book(key);        
        book(p);
    }
}

void Manager::operator()(core::TargetQuotes const & target_quotes) {
    log::info<2>("oms TargetQuotes {}", target_quotes);    
    assert(!target_quotes.account.empty());
    
    core::Market market {
        .market = target_quotes.market,        
        .symbol = target_quotes.symbol,
        .exchange = target_quotes.exchange,
    //    .account = target_quotes.account
    };

    auto [info, _] = core.markets.emplace_market(market);

    // FIXME: this should not be problem, we should get rid on tick_size dependency (sorted map?)
    if(std::isnan(info.tick_size)) {
        return;
    }

    // resolve empty portfolio into one associated with account (if any)

    //core::PortfolioIdent portfolio = target_quotes.portfolio;
    //if(!portfolio) {
    //    core.portfolios.get_portfolio_by_account(target_quotes.account, target_quotes.exchange, [&](core::Portfolio const& p) {
    //        portfolio = p.portfolio;
    //    });
    //}

    auto key = oms::Market { 
        .account = target_quotes.account,
        .portfolio = target_quotes.portfolio,
        .strategy = target_quotes.strategy,
    }.merge(info);

    auto [book, is_new] = emplace_book(key);
    book.set_tick_size(info.tick_size);

    assert(book.exchange == target_quotes.exchange);
    assert(book.symbol == target_quotes.symbol);

    for(auto& [price_index, quote]: book.bids) {
        quote.target_quantity = 0;
        quote.exec_inst = {};
    }
    for(auto& quote: target_quotes.buy) {
        if(!is_empty_value(quote)) {
            auto [level,is_new] = book.emplace_level(Side::BUY, quote.price);
            level.target_quantity = quote.volume;
            level.exec_inst = quote.exec_inst;
        }
    }
    for(auto& [price_index, quote]: book.asks) {
        quote.target_quantity = 0;
        quote.exec_inst = {};
    }
    for(auto& quote: target_quotes.sell) {
        if(!is_empty_value(quote)) {
            auto [level,is_new] = book.emplace_level(Side::SELL, quote.price);
            level.target_quantity = quote.volume;
            level.exec_inst = quote.exec_inst;
        }
    }

    get_books([&](oms::Book & book, core::market::Info const& info) {
        process(book, info);
    });
}


bool Manager::is_throttled(oms::Book& book, RequestType req) {
    return false;
}

bool Manager::can_create(oms::Book& book, core::market::Info const& info, const core::TargetOrder & target_order) {
    if(book.pending[target_order.side==Side::SELL]>0)
        return false;

    if(is_throttled(book, roq::RequestType::CREATE_ORDER)) {
        return false;
    }
    if(target_order.quantity < info.min_trade_vol) {
        return false;
    }
    return true;
}

bool Manager::can_cancel(oms::Book& book, core::market::Info const& info, oms::Order& order) {
    if(!order.confirmed.version)
        return false;   // still pending
    if(order.is_pending())
        return false;   // something in-flight
    return true;
}

/// can_modify_price, can_modify_volume
std::pair<bool,bool> Manager::can_modify(oms::Book& book, core::market::Info const& info, oms::Order& order) {
    if(book.ban_modify)
        return {false,false};
    //if(!order.is_confirmed())
    if(!order.confirmed.version)    
        return {false,false};   // still pending    

    if(order.rejects_count)
        return {false, false};  // cancel-only policy for rejected orders

    if(order.is_pending())
        return {false, false};   // something in-flight (modify chaining...)

    if(!core.gateways.is_ready(roq::Mask{roq::SupportType::MODIFY_ORDER}, book.trade_gateway_id, book.account))
        return {false,false};

    return {true,true};
}

void Manager::process(oms::Book& book, core::market::Info const& info) {
    if(book.account.empty())
        return;
    std::chrono::nanoseconds now = this->now();
    auto mask = roq::Mask{roq::SupportType::CREATE_ORDER, roq::SupportType::CANCEL_ORDER};
    
    bool trade_gateway_resolved = resolve_trade_gateway(book);

    bool ready = core.gateways.is_ready(mask, book.trade_gateway_id, book.account) && trade_gateway_resolved;
    log::info<2>("OMS process now {} symbol {} exchange {} account {} ban {} ready {} trade_gateway.{} {} tick_size {}",
         now, book.symbol, book.exchange, book.account, book.ban_until.count() ? (book.ban_until-now).count()/1E9:NaN, ready, book.trade_gateway_id, book.trade_gateway_name, info.tick_size);
    
    if(!ready) {
        return;
    }

    if(std::isnan(info.tick_size)) {
        return;
    }

    book.set_tick_size(info.tick_size);
    
    if(now < book.ban_until) {
        return;
    } else {
        book.ban_until = {};
    }

    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = book.get_levels(side);
        for(auto& [price_index, level]: levels) {
            level.expected_quantity = 0;
            level.confirmed_quantity = 0;
        }
    }

again:

    book.pending = {0,0};
    
    for(auto& [order_id, order] : book.orders) {
        if(order.confirmed.status == OrderStatus::COMPLETED || order.confirmed.status == OrderStatus::CANCELED)
            continue;
        assert(order.confirmed.status!=OrderStatus::COMPLETED);
        assert(order.confirmed.status!=OrderStatus::CANCELED);
        assert(!std::isnan(order.expected.quantity));
        if(order.is_pending())
            book.pending[order.side==Side::SELL]++;

        //assert(utils::compare(order.expected.quantity,0)==std::strong_ordering::greater);
        auto [level,is_new_level] = book.emplace_level(order.side, order.expected.price);
        assert(!std::isnan(level.expected_quantity));          
        level.expected_quantity += order.expected.quantity;
        level.confirmed_quantity += order.confirmed.quantity;
        log::info<2>("OMS order_state order_id={}.{}.{} side={} "
             " price={} quantity={} " 
            " p.req={} p.price={}  p.quantity={} "
            " c.status.{} c.price={} c.quantity={} external_id={}"
            " symbol={} exchange={} market={}",
            order.order_id, order.pending.version, order.confirmed.version,order.side,
            order.expected.price,order.expected.quantity,
            order.pending.type, order.pending.price,order.pending.quantity,
            order.confirmed.status, order.confirmed.price, order.confirmed.quantity,
            order.external_order_id, book.symbol, book.exchange, book.market);
    }

    log::info<2>("OMS order_state BUY count {} pending {}  SELL count {} pending {}", 
        book.bids.size(), book.pending[0],
        book.asks.size(), book.pending[1]);

    for(Side side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = (side==Side::BUY) ? book.bids : book.asks;
        for(const auto& [price_index, level]: levels) {
            log::info<2>("OMS level_state {} {} target {} expected {} confirmed {}", side, level.price, level.target_quantity, level.expected_quantity, level.confirmed_quantity);
        }
    }
    std::size_t orders_count = book.orders.size();
    for(auto& [order_id, order] : book.orders) {
        auto [level,is_new_level] = book.emplace_level(order.side, order.expected.price);
        if(utils::compare(level.expected_quantity, level.target_quantity)==std::strong_ordering::greater) {
            auto [can_modify_price, can_modify_volume] = can_modify(book, info, order);
            auto can_cancel_flag = can_cancel(book, info, order);
            log::info<2>("OMS can_modify (price {} qty {}) can_cancel {} "
            " order_id={}.{}.{} side={} req={}  price={}  quantity={}"
            " c.req={}, c.status.{} c.price={}  c.quantity={} "
            " p.req={}, p.status.{} p.price={}  p.quantity={} "
            " external_id={} "
            " symbol={} exchange={} market={}", 
            can_modify_price, can_modify_volume, can_cancel_flag,
            order.order_id, order.pending.version, order.confirmed.version,
            order.side,order.pending.type, order.pending.price,order.pending.quantity,
            order.confirmed.type, order.confirmed.status, order.confirmed.price, order.confirmed.quantity, 
            order.pending.type, order.pending.status, order.pending.price, order.pending.quantity, 
            order.external_order_id, book.symbol, book.exchange, book.market);

            if(can_modify_price || can_modify_volume) {
                const auto& levels = book.get_levels(order.side);                
                for(const auto& [new_price_index, new_level]:  levels) {
                    assert(!std::isnan(new_level.target_quantity));
                    assert(!std::isnan(new_level.expected_quantity));                    
                    assert(!std::isnan(order.confirmed.quantity));
                    if(!can_modify_price && utils::compare(new_level.price, order.expected.price)!=std::strong_ordering::equal)
                        continue;
                    if(!can_modify_volume) {
                        if(utils::compare(new_level.target_quantity,new_level.expected_quantity+order.confirmed.quantity)!=std::strong_ordering::less) {
                            /// just move as a whole to location with enough target quantity hole
                            modify_order(book, order, core::TargetOrder {
                                .quantity = order.confirmed.quantity,    // can_modify_qty?new_level.target_quantity-new_level.expected_quantity : order.confirmed.quantity
                                .price = new_level.price,
                            });
                            goto again; // FIXME: should update expected vol here instead of recalculating all levels
                        }
                    } else {
                        if(utils::compare(new_level.target_quantity,new_level.expected_quantity)==std::strong_ordering::greater) {
                            /// just move as a whole to location with  target quantity hole changing the volume accordingly
                            modify_order(book, order, core::TargetOrder {
                                .quantity = new_level.target_quantity - new_level.expected_quantity,
                                .price = new_level.price,
                            });
                            goto again; // FIXME: should update expected vol here instead of recalculating all levels
                        }
                    }
                }
            }
            if(can_cancel_flag) {
                this->cancel_order(book, order);
                goto again; // FIXME: should update expected vol here instead of recalculating all levels
            }
            // can't do anything with this order (yet)
        }
    }
    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = book.get_levels(side);//(side==Side::BUY) ? bids : asks;
        for(auto& [price_index, level] : levels) {
            assert(!std::isnan(level.target_quantity));
            assert(!std::isnan(level.expected_quantity));
            if(utils::compare(level.target_quantity, level.expected_quantity)==std::strong_ordering::greater) {
                auto target_order = core::TargetOrder {
                    .market = book.market,
                    .side = side,
                    .quantity = level.target_quantity - level.expected_quantity,
                    .price = level.price,
                    .exec_inst = level.exec_inst
                };
                if(can_create(book, info, target_order)) {
                    //queue_.push_back(target_order); 
                    create_order(book, target_order);
                }
            }
        }
    }
    
    for(auto side: std::array {Side::BUY, Side::SELL}) {
        auto& levels = book.get_levels(side);

        for(auto it = levels.begin(); it!=levels.end();) {
            auto& level = it->second;
            assert(!std::isnan(level.price));
            assert(!std::isnan(level.target_quantity));
            assert(!std::isnan(level.expected_quantity));            
            if(roq::utils::compare(level.target_quantity,0.) == std::strong_ordering::equal && 
                roq::utils::compare(level.expected_quantity, 0.) == std::strong_ordering::equal && 
                roq::utils::compare(level.confirmed_quantity, 0.) == std::strong_ordering::equal) {
                auto price = level.price;
                
                //FIXME: abseil stle 
                //levels.erase(it++);
                it = levels.erase(it);
                log::info<2>("OMS erase_level {} price={} count={}", side, price, levels.size());
            } else {
                it++;
            }
        }
    }
}

oms::Order& Manager::create_order(oms::Book& book, const core::TargetOrder& target) {
    assert(book.market == target.market);
    auto const mask = roq::Mask<roq::SupportType>{roq::SupportType::MODIFY_ORDER};
    assert(core.gateways.is_ready(mask, book.trade_gateway_id, book.account));
    auto order_id = ++max_order_id;
    assert(book.orders.find(order_id)==std::end(book.orders));
    auto& order = book.orders[order_id] = oms::Order {
        .order_id = order_id,
        .side = target.side,
        //.price = target.price,
        //.quantity = target.quantity,
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
        .account = book.account,
        .order_id = order_id,
        .exchange = book.exchange,
        .symbol = book.symbol,
        .side = target.side,
        //.execution_instructions = target.execution_instructions,
        .order_type = OrderType::LIMIT,
        .time_in_force = TimeInForce::GTC,
        .execution_instructions = execution_instructions,                
        .quantity = target.quantity,        
        .price = target.price, 
        .routing_id = routing_id_v,
        .strategy_id = book.strategy,        
    };

    market_by_order_[order_id] = oms::Market {
        .market = book.market,
        .symbol = book.symbol,
        .exchange = book.exchange,
        .account = book.account,
        .strategy = book.strategy,
        .trade_gateway_name = book.trade_gateway_name
    };

    log::info<1>("OMS create_order {{ order_id={}.{} side={} price={} qty={} symbol={} exchange={} market {}, account={}, execution_instructions={} }}", 
        order_id, order.pending.version, target.side, target.price, target.quantity, 
        book.symbol, book.exchange, book.market, book.account, 
        create_order.execution_instructions);
    dispatcher.send(create_order, book.trade_gateway_id);
    return order;
}

void Manager::modify_order(oms::Book& book, oms::Order& order, const core::TargetOrder & target) {
    const auto mask = roq::Mask<roq::SupportType>{roq::SupportType::MODIFY_ORDER};
    assert(core.gateways.is_ready(mask, book.trade_gateway_id, book.account));
    ++order.pending.version;
    order.pending.type = RequestType::MODIFY_ORDER;
    order.pending.price = target.price;
    order.pending.quantity = target.quantity;

    auto modify_order = ModifyOrder {
        .account = book.account,
        .order_id = order.order_id,
        .quantity = target.quantity,
        .price = target.price,
        .version = order.pending.version,
        .conditional_on_version = order.confirmed.version
    };
    
    log::info<1>("OMS modify_order {{ order_id={}.{}/{}, side={} price={}->{} qty={}->{} external_id={} market={} symbol={} exchange={} account={} }}", 
        modify_order.order_id, modify_order.version, modify_order.conditional_on_version, 
        order.side, order.confirmed.price, modify_order.price, order.expected.quantity, 
        modify_order.quantity, order.external_order_id, 
        book.market, book.symbol, book.exchange, book.account);
    
    order.expected = order.pending;

    dispatcher.send(modify_order, book.trade_gateway_id);
}

void Manager::cancel_order(oms::Book& book, oms::Order& order) {
    const auto mask = roq::Mask<roq::SupportType>{roq::SupportType::CANCEL_ORDER};
    assert(core.gateways.is_ready(mask, book.trade_gateway_id, book.account));
    assert(book.orders.find(order.order_id)!=std::end(book.orders));
    ++order.pending.version;
    order.pending.type = RequestType::CANCEL_ORDER;
    order.pending.price = order.expected.price;
    order.pending.quantity = 0;

    auto cancel_order = CancelOrder {
        .account = book.account,
        .order_id = order.order_id,
        .version = order.pending.version,
        .conditional_on_version = order.confirmed.version,
    };
    
    log::info<1>("OMS cancel_order {{ order_id={}.{}/{}, side={} price={} qty={} external_id={} market={} symbol={} exchange={} account={} }}", 
        cancel_order.order_id, cancel_order.version, cancel_order.conditional_on_version, 
        order.side, order.expected.price, order.expected.quantity, order.external_order_id, book.market, book.symbol, book.exchange, book.account);
    
    order.expected = order.pending;

    dispatcher.send(cancel_order, book.trade_gateway_id);
}


void Manager::order_create_reject(oms::Book& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    assert(u.request_type == RequestType::CREATE_ORDER);
    order.reject_version = u.version;
    order.reject_error = u.error;
    order.reject_reason = u.text;
    order.rejects_count++;
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

void Manager::order_modify_reject(oms::Book& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    assert(u.request_type == RequestType::MODIFY_ORDER);
    order.reject_version = u.version;
    order.reject_error = u.error;
    order.reject_reason = u.text;
    order.rejects_count++;
    order.expected = order.confirmed;
    order.pending.type = RequestType::UNDEFINED;
}

void Manager::order_cancel_reject(oms::Book& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    assert(u.request_type == RequestType::CANCEL_ORDER);
    order.expected = order.confirmed;
    order.pending.type = RequestType::UNDEFINED;    
}

void Manager::order_fwd(oms::Book& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
}

void Manager::order_accept(oms::Book& market, oms::Order& order, const OrderAck& u) {
    assert(order.order_id == u.order_id);
    order.accept_version = u.version;
}

void Manager::order_confirm(oms::Book& market, oms::Order& order, const OrderUpdate& u) {
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
    order_fills(market, u.side, u.last_traded_price, fill_size);
}


bool Manager::reconcile_positions(oms::Book& book) {
    bool is_downloading = core.gateways.is_downloading(book.trade_gateway_id);
    if(is_downloading)
        return false;
    if(now()-book.last_position_modify_time > std::chrono::seconds{3}) {
        auto delta =  book.position_by_account - book.position_by_orders;    // stable
        if( !std::isnan(delta) && utils::compare(delta, 0.)!=std::strong_ordering::equal) {
            auto prev_by_orders = book.position_by_orders;
            //book.position_by_orders = book.position_by_account;
            log::info<1>("OMS reconcile_positions (ignored) by_orders {} -> {} position_by_account {} delta {}",
                prev_by_orders, book.position_by_orders, book.position_by_account, delta);
            
            return true;
        }
    }
    return false;
}

void Manager::order_fills(oms::Book& book, roq::Side side, double price, double fill_size) {
    assert(!std::isnan(fill_size));
    if(utils::compare(fill_size, 0.)==std::strong_ordering::greater) {
        switch(side) {
            case Side::BUY:     book.position_by_orders += fill_size; break;
            case Side::SELL:    book.position_by_orders -= fill_size; break;
            default: assert(false);
        }

        core::Trade trade {
            .side = side,
            .price = price,
            .quantity = fill_size,
            .market = book.market,
            .symbol = book.symbol,
            .exchange = book.exchange,
            .portfolio = book.portfolio,
        };

        //if(core.portfolios.position_source!=core::PositionSource::ORDERS) {
            // NOTE: since PositionUpdate is asyncronous, we should prevent adding new orders (FIXME: we should be able to cancel though)
        book.last_position_modify_time = now();
        if(book.post_fill_timeout!=core::Duration{}) {
            book.ban_until = now() + book.post_fill_timeout;
        }
        //}
        // to portfolio::Manager
        handler_(trade);

    }
}

//void Manager::operator()(roq::Event<core::ExposureUpdate> const& event) {
//    // FIXME: exposure update comes into oms from outside?
//    handler_(event);
//}

void Manager::order_complete(oms::Book& market, oms::Order& order, const OrderUpdate& u) {
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
    order_fills(market, u.side, u.last_traded_price, fill_size);
}

void Manager::order_canceled(oms::Book& book, oms::Order& order, const OrderUpdate& u) {
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
    erase_order(book, order_id);
    if(book.post_cancel_timeout!=core::Duration{}) {
        book.ban_until = now() + book.post_cancel_timeout;
    }
}

void Manager::operator()(roq::Event<Timer> const& event) {
    now_ = event.value.now;
    if(now_ - last_process_ >= std::chrono::seconds(1)) {
        get_books([&](oms::Book& market, core::market::Info const& info) {
            //    if(!is_downloading(state.gateway_id))
            process(market, info);
            reconcile_positions(market);
        });
        last_process_ = now_;        
    }
/*    for(auto& [market_id, market] : books_) {
        //if(!is_downloading(book.gateway_id))
        book.reconcile_positions(market);
    }
*/
}

void Manager::operator()(roq::Event<GatewayStatus> const& event) {
    log::info<2>("OMS GatewayStatus {}, books_size {} erase_all_orders_on_gateway_not_ready {}", event, books_.size(), core::Flags::erase_all_orders_on_gateway_not_ready());

    get_books([&](oms::Book& book, core::market::Info const& info) {
        if (event.message_info.source_name == book.trade_gateway_name) {
            book.trade_gateway_id = event.message_info.source;
            log::info<1>("oms assigned trade_gateway_id {} to trade_gateway {}",
                    book.trade_gateway_id, book.trade_gateway_name);
        }
        if(!event.value.account.empty() && book.account == event.value.account) {
            if(core::Flags::erase_all_orders_on_gateway_not_ready()) {
                if(!event.value.available.has_all(roq::Mask{roq::SupportType::CREATE_ORDER, roq::SupportType::CANCEL_ORDER})) {
                    log::info<1>("OMS GatewayStatus available {} account {} exchange {} symbol {} market {} erase_all_orders", 
                        event.value.available, book.account, book.exchange, book.symbol, book.market);
                    erase_all_orders(book);
                }
            }
        }
    });
    Base::operator()(event);
}

void Manager::erase_all_orders(oms::Book& book) {
    for(auto& [order_id, order]: book.orders) {
        log::info<1>("OMS erase_all_orders order_id={}.{} side={} price={} remaining_quantity={} status={} symbol={} exchange={} market={}", 
            order_id, order.pending.version, order.side, order.expected.price, order.expected.quantity,
            order.confirmed.status, 
            book.symbol, book.exchange, book.market);    
        market_by_order_.erase(order_id);
    }
    book.orders.clear();
}

bool Manager::erase_order(oms::Book& book, uint64_t order_id) {
    auto iter = book.orders.find(order_id);
    assert(iter!=std::end(book.orders));
    if(iter==std::end(book.orders))
        return false;
    auto& order = iter->second;
    log::info<1>("OMS erase_order order_id={}.{} side={} price={} remaining_quantity={} status={} symbol={} exchange={} market={}", 
        order_id, order.pending.version, order.side, order.expected.price, order.expected.quantity, 
        order.confirmed.status, 
        book.symbol, book.exchange, book.market);
    book.orders.erase(iter);
    market_by_order_.erase(order_id);
    return true;
}

void Manager::operator()(roq::Event<OrderUpdate> const& event) {
    auto& u = event.value;
    oms::Market key = {
        .symbol = u.symbol,
        .exchange = u.exchange,
        .account = u.account,
        .strategy = u.strategy_id
    };
    if(!get_book(key, [&](oms::Book& book, core::market::Info const & info) {

        auto [order,is_new_order] =  book.emplace_order(u.order_id);
        assert(order.order_id == u.order_id);

        log::info<1>("OMS order_update is_new_order={} order_id={}.{} side={} price={} remaining_quantity={} status={} update_type={} symbol={} exchange={} market={}", 
            is_new_order, u.order_id, u.max_accepted_version, u.side, u.price, u.remaining_quantity, u.order_status, u.update_type, u.symbol, u.exchange, book.market);

        if(u.update_type==roq::UpdateType::STALE) {
            erase_order(book, u.order_id);
        } else if(is_new_order) {
            order.side = u.side;
            order.order_id = u.order_id;
            //order.price = u.price;
            order.remaining_quantity = u.remaining_quantity;
            order.traded_quantity = u.traded_quantity;
            order.confirmed.type = RequestType::CREATE_ORDER;
            order.confirmed.version = u.max_accepted_version;
            order.confirmed.status = u.order_status;
            order.confirmed.price = u.price;
            order.confirmed.quantity = u.remaining_quantity;
            order.external_order_id = u.external_order_id;
            order.pending.version = u.max_response_version;
            order.pending.type = RequestType::UNDEFINED;
            order.expected = order.confirmed;
            market_by_order_[order.order_id] = oms::Market {
                .market = book.market,
                .symbol = book.symbol,
                .exchange = book.exchange,
                .account = book.account,
                .strategy = book.strategy,
                .trade_gateway_name = book.trade_gateway_name,                
            };
            
            if(u.order_status!=OrderStatus::WORKING) {
                // keep only working
                erase_order(book, order.order_id);
            }
        } else {
            if(u.order_status==OrderStatus::WORKING) {
                order_confirm(book, order, u);
            } else if(u.order_status == OrderStatus::COMPLETED) {
                order_complete(book, order, u);
            } else if(u.order_status == OrderStatus::CANCELED) {
                order_canceled(book, order, u);
            } else if(u.order_status == OrderStatus::REJECTED) {
                erase_order(book, order.order_id);
            }
        }
        process(book, info);
    })) {
        log::info<1>("oms order_update not_found key {}", key);
    }
}

bool Manager::resolve_trade_gateway(oms::Book& book) {
    if(book.trade_gateway_name.empty()) {
        book.trade_gateway_name = get_trade_gateway(oms::Market {
            .market=book.market,            
            .symbol=book.symbol,
            .exchange=book.exchange,    
            .account=book.account,
        });
        book.trade_gateway_id = -1;
    }
    if(book.trade_gateway_id<0) {
        book.trade_gateway_id = core.gateways.get_source(book.trade_gateway_name);
        log::info("oms resolved trade_gateway_id to {} for gateway {} for book {}@{}", book.trade_gateway_id, book.trade_gateway_name, book.symbol, book.exchange);
    }
    return book.trade_gateway_id>=0;
}

std::pair<oms::Book&, bool> Manager::emplace_book(oms::Market const & key) {    
    assert(key.strategy);
    assert(!key.account.empty());
    auto& by_account = books_[key.strategy];
    auto& by_market = by_account[key.account];
    auto iter = by_market.find(key.market);
    if(iter!=std::end(by_market)) {
        return {iter->second, false};
    } else {
        core::MarketIdent market = key.market;
        if(!market) {
            assert(!key.exchange.empty());
            assert(!key.symbol.empty());
            auto [info, _] = core.markets.emplace_market({
                .symbol=key.symbol,
                .exchange=key.exchange
            });
            market = info.market;
        }
        auto [iter_1, is_new] = by_market.try_emplace(market);
        oms::Book& book = iter_1->second;
        book.market = market;
        book.exchange = key.exchange;
        book.symbol = key.symbol;
        book.account = key.account;
        book.strategy = key.strategy;
        if(!key.trade_gateway_name.empty()) {
            book.trade_gateway_name = key.trade_gateway_name;
        } else {
            book.trade_gateway_name = get_trade_gateway(key);
        }
        assert(!book.trade_gateway_name.empty());

        book.last_position_modify_time = now();
        // FIXME: no accounts ?
        
        //if(market_2.account.empty()) {
        //    core_.accounts.get_account(market.exchange, [&](auto& account) {
        //        market_2.account = account;
        //    });
        //}
        log::info<2>("oms emplace_book symbol {} exchange {} account {} market {} strategy {}", 
            book.symbol, book.exchange, book.account, book.market, book.strategy);
        return {book, is_new};
    }
}


void Manager::operator()(roq::Event<OrderAck> const& event) {
    auto& u = event.value;
    auto iter = market_by_order_.find(u.order_id);
    if(iter==market_by_order_.end())
    {
        log::info<1>("OMS order_ack not_found {} order_id={}.{} side={}, status={} symbol={} exchange={}", 
            u.request_type, u.order_id, u.version, u.side, u.request_status, u.symbol, u.exchange);
        return;
    }
    oms::Market& key = iter->second;
    if(!get_book(key, [&](oms::Book & market, core::market::Info const& info) {

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
                    market.ban_until = std::max(market.ban_until, now() + reject_timeout_);
                } else {
                    process(market, info);
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
    })) {
        log::info<1>("OMS order_ack book not_found {} order_id={}.{} side={}, status={} key {}", 
            u.request_type, u.order_id, u.version, u.side, u.request_status, key);            
    }
}


void Manager::operator()(Event<PositionUpdate> const & event) {
    Base::operator()(event);
    auto& u = event.value;    
    log::info<2>("PositionUpdate {}", event);    
    core.portfolios.get_portfolio_by_account(u.account, u.exchange, [&](core::Portfolio& p) {
        oms::Market key = {
            .symbol = u.symbol,
            .exchange = u.exchange,
            .account = u.account,
            .strategy = p.portfolio, //NOTE: portfolio_id == strategy_id
        };
        get_book(key,[&](oms::Book& book, core::market::Info const& info) {
            auto new_position = u.long_quantity - u.short_quantity;
            book.position_by_account = new_position; 
            auto gateway_id = event.message_info.source;
            bool is_downloading = core.gateways.is_downloading(gateway_id);    
            if(!is_downloading && core.portfolios.position_source==core::PositionSource::PORTFOLIO) {
                book.position_by_account = new_position;
                book.last_position_modify_time = now();
                log::info<1>("OMS position_update downloading {} account {} position_by_orders {} position_by_account {} symbol {} exchange {} market {}",
                    is_downloading, book.account, book.position_by_orders, book.position_by_account,
                    book.symbol, book.exchange, book.market);

            }
        });
    });
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
    log::info<2>("oms {}", event);
}

void Manager::operator()(roq::Event<DownloadBegin> const& event) {
    log::info<2>("oms {}", event);
//    position_by_account_.erase(event.value.account);
//    ready_by_gateway_[event.message_info.source] = false;
    auto& u = event.value;
    if(u.account.empty())
        return;
    get_books([&](oms::Book& market, core::market::Info const& info) {
        if(market.account == u.account) {
            //if(position_snapshot==core::PositionSnapshot::ACCOUNT) {
                market.position_by_orders = 0;
                market.position_by_account = 0;
            //}
            erase_all_orders(market);
            log::info<1>("OMS position_download_begin account {} market.{} {}@{} portfolio.{} {} position_by_orders {} position_by_account {} erase_all_orders",
                market.account, market.market,market.symbol, market.exchange, 
                market.portfolio, market.portfolio_name, 
                market.position_by_orders, market.position_by_account);
        }
    });
}

void Manager::operator()(roq::Event<DownloadEnd> const& event) {
    log::info<2>("DownloadEnd {}", event);
    
    max_order_id  = std::max(max_order_id, event.value.max_order_id);
    auto& u = event.value;
    if(!u.account.empty() /*&& position_snapshot==core::PositionSnapshot::ACCOUNT*/) {
        get_books([&](oms::Book & book, core::market::Info const& info) {
            if(book.account==u.account) {
                book.position_by_orders = book.position_by_account;
                book.last_position_modify_time = now();
                log::info<1>("OMS position_snapshot account {} portfolio.{} {} position_by_orders = position_by_account = {}  market {}",  
                        u.account, book.portfolio, book.portfolio_name, book.position_by_orders, book.market);
            }
        });
    }
}

void Manager::operator()(roq::Event<RateLimitTrigger> const& event) {
    auto& u = event.value;
    log::info<1>("RateLimitTrigger {}", u);
    switch(u.buffer_capacity) {
       case roq::BufferCapacity::HIGH_WATER_MARK: {
       } break;
       case roq::BufferCapacity::FULL: {
           get_books([&](oms::Book &market, core::market::Info const& info){
               if(event.message_info.source_name == market.exchange) {
                auto ban_until = market.ban_until = std::max(now(), u.ban_expires);
                log::info<1>("RateLimitTrigger ban_until {} ({}s) exchange {} market {}", 
                    market.ban_until, ban_until.count() ? (ban_until-this->now()).count()/1E9:NaN, market.exchange, market.market);
               }
            });
       } break;
       default: break;
   }
}

std::string_view Manager::get_trade_gateway(oms::Market const& market) {
    auto iter_1 = trade_gateway_by_account_by_exchange_.find(market.exchange);
    if(iter_1 == std::end(trade_gateway_by_account_by_exchange_))
        return "";
    auto iter_2 = iter_1->second.find(market.account);
    if(iter_2 == std::end(iter_1->second))
        return "";
    return iter_2->second;
}


void Manager::clear() {
    trade_gateway_by_account_by_exchange_.clear();
}

void Manager::configure(const config::TomlFile& config, config::TomlNode root) {
    clear();
    
    auto node = root["oms"];

    static constexpr core::Integer MIN_REJECT_TIMEOUT_MS = 100;

    config.get_nodes(root, "account", [&](auto node) {
      auto account = config.get_string(node, "account");
      auto trade_gateway_name = config.get_string(node, "trade_gateway");
      auto exchange = config.get_string(node, "exchange");
      trade_gateway_by_account_by_exchange_[exchange][account] = trade_gateway_name;
    });

    this->reject_timeout_ =  std::chrono::milliseconds { config.get_value_or(node, "reject_timeout", MIN_REJECT_TIMEOUT_MS) };
}


} // namespace roq::oms