#include "roq/core/quotes.hpp"
#include "roq/core/best_quotes.hpp"

namespace roq::core {

bool BestQuotes::update(core::Quotes const &rhs) {
  if (rhs.buy.empty()) {
    buy = {};
  } else {
    buy = rhs.buy[0];
  }
  if (rhs.sell.empty()) {
    sell = {};
  } else {
    sell = rhs.sell[0];
  }
  return true; // FIXME: return if changed
}

void BestQuotes::clear() {
  buy.clear();
  sell.clear();
}

core::Double exposure_by_quotes(core::BestQuotes const &self) {
  return self.buy.volume - self.sell.volume;
}

} // namespace roq::core