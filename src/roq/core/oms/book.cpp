// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/oms/book.hpp" 
#include "roq/numbers.hpp"
#include "roq/utils/compare.hpp"
#include <roq/logging.hpp>
#include "roq/core/string_utils.hpp"

namespace roq::core::oms {
using namespace std::literals;

void Book::set_tick_size(core::Double new_tick_size) {
  if(tick_size.empty()) {
      this->tick_size = new_tick_size;
    }
    assert(this->tick_size.empty() || this->tick_size == new_tick_size);
}

std::pair<oms::Level&, bool> Book::emplace_level(Side side, core::Price price) {
    assert(!std::isnan(price));
    assert(!tick_size.empty());
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

roq::core::Hash<LevelIdent, oms::Level> &Book::get_levels(Side side) {
  switch (side) {
  case Side::BUY:
    return bids;
  case Side::SELL:
    return asks;
  default:
    throw roq::RuntimeError("UNEXPECTED");
  }
}

std::pair<oms::Order &, bool> Book::emplace_order(OrderIdent order_id) {
  auto iter = orders.find(order_id);
  if (iter == std::end(orders)) {
    auto &order = orders[order_id];
    order.order_id = order_id;
    return {order, true};
  } else {
    return {iter->second, false};
  }
}

void Book::operator()(roq::Parameter const& p) {
  auto [prefix, label] = core::split_prefix(p.label,':');
  if(label=="post_fill_timeout"sv) {            
    assert(!p.account.empty());
    post_fill_timeout = std::chrono::milliseconds { core::parse_uint32(p.value) };
  }
}

} // roq::oms