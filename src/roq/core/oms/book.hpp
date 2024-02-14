// (c) copyright 2023 Mikhail Mitkevich
#pragma once 

#include "roq/core/types.hpp"
#include "roq/core/oms/order.hpp"
#include "roq/core/oms/order_version.hpp"
#include "roq/core/hash.hpp"
#include "roq/core/oms/market.hpp"
#include "roq/core/oms/level.hpp"
#include <chrono>

namespace roq::core::oms {

struct Manager;
using OrderIdent = core::OrderIdent;
using LevelIdent = int64_t;
using MarketIdent = core::MarketIdent;


struct Book {
    MarketIdent market {};
    roq::Symbol symbol;
    roq::Exchange exchange;
    roq::Account account;
    core::PortfolioIdent portfolio {};    
    roq::Account portfolio_name;
    core::StrategyIdent strategy {};

    core::Hash<OrderIdent, oms::Order> orders;
    core::Hash<LevelIdent, oms::Level> bids;
    core::Hash<LevelIdent, oms::Level> asks;

    core::Double tick_size {};
    uint32_t trade_gateway_id = (uint32_t)-1;
    
    core::Duration post_fill_timeout = std::chrono::milliseconds{100};
    core::TimePoint ban_until {};
    
    std::array<uint16_t,2> pending = {0,0};

    // begin reconcile
    double position_by_orders = 0;
    double position_by_account = 0;
    core::TimePoint last_position_modify_time;
    // end reconcile
public:
  std::pair<oms::Level &, bool> emplace_level(Side side, core::Price price, core::Double new_tick_size);

  core::Hash<LevelIdent, oms::Level> &get_levels(Side side);

  // order&, is_new
  std::pair<oms::Order &, bool> emplace_order(OrderIdent order_id);

  bool get_order(OrderIdent order_id, std::invocable<oms::Order&> auto &&fn) {
    return orders.get_value(order_id, std::move(fn));
  }
};

}