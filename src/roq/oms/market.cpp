// (c) copyright 2023 Mikhail Mitkevich
#include "roq/oms/market.hpp" 
#include "roq/numbers.hpp"
#include "roq/utils/compare.hpp"
#include <roq/logging.hpp>

namespace roq::oms {

std::pair<oms::Level&, bool> Market::emplace_level(Side side, double price) {
    assert(!std::isnan(price));
    assert(!std::isnan(tick_size));
    LevelIdent index = std::roundl(price/tick_size);
    auto &levels = this->get_levels(side);
    auto iter = levels.find(index);
    if (iter != std::end(levels)) {
    assert(utils::compare(iter->second.price, price) ==
            std::strong_ordering::equal);
    return {iter->second, false};
    } else {
    auto &level = levels[index];
    log::info<2>("OMS new_level {} price={} count={}", side, price,
                levels.size());
    level.price = price;
    return {level, true};
    }
}

roq::core::Hash<LevelIdent, oms::Level> &Market::get_levels(Side side) {
  switch (side) {
  case Side::BUY:
    return bids;
  case Side::SELL:
    return asks;
  default:
    throw roq::RuntimeError("UNEXPECTED");
  }
}

std::pair<oms::Order &, bool> Market::emplace_order(OrderIdent order_id) {
  auto iter = orders.find(order_id);
  if (iter == std::end(orders)) {
    auto &order = orders[order_id];
    order.order_id = order_id;
    return {order, true};
  } else {
    return {iter->second, false};
  }
}

} // roq::oms