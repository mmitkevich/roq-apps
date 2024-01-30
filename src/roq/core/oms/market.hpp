// (c) copyright 2023 Mikhail Mitkevich
#pragma once 

#include "roq/core/types.hpp"
#include "roq/core/oms/order.hpp"
#include "roq/core/oms/order_version.hpp"
#include "roq/core/hash.hpp"
#include "roq/core/oms/level.hpp"

namespace roq::core::oms {

struct Manager;
using OrderIdent = core::OrderIdent;
using LevelIdent = int64_t;
using MarketIdent = core::MarketIdent;

struct Market {
    core::Hash<OrderIdent, oms::Order> orders;
    core::Hash<LevelIdent, oms::Level> bids;
    core::Hash<LevelIdent, oms::Level> asks;

    roq::Exchange exchange;
    uint32_t trade_gateway_id = (uint32_t)-1;
    roq::Symbol symbol;
    roq::Account account;
    double tick_size = NAN;
    double min_trade_vol = NAN;
    core::Duration ban_post_fill = {};
    MarketIdent market {};
    core::TimePoint ban_until {};
    std::array<uint16_t,2> pending = {0,0};
    double position_by_orders = 0;
    double position_by_account = 0;
    core::TimePoint last_position_modify_time;
    core::PortfolioIdent portfolio {};
      roq::Account portfolio_name;
public:
  std::pair<oms::Level &, bool> emplace_level(Side side, double price);

  core::Hash<LevelIdent, oms::Level> &get_levels(Side side);

  // order&, is_new
  std::pair<oms::Order &, bool> emplace_order(OrderIdent order_id);

  bool get_order(OrderIdent order_id, std::invocable<oms::Order&> auto &&fn) {
    return orders.get_value(order_id, std::move(fn));
  }
};

}