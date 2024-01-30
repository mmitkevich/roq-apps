#include "roq/core/best_quotes/manager.hpp"
#include "roq/core/quotes.hpp"

namespace roq::core::best_quotes {

std::pair<core::BestQuotes &, bool> Manager::emplace_quotes(core::MarketIdent market) {
  auto [iter, is_new] = cache.try_emplace(market);
  return {iter->second, is_new};
}

core::BestQuotes& Manager::set_quotes(core::MarketIdent market, core::BestQuotes const& quotes) {
  auto& item =  cache[market];
  item = quotes;
  return item;
}

} // namespace roq::core