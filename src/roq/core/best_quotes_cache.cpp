#include "roq/core/best_quotes_cache.hpp"
#include "roq/core/quotes.hpp"

namespace roq::core {

std::pair<core::BestQuotes &, bool> BestQuotesCache::emplace_quotes(core::MarketIdent market) {
  auto [iter, is_new] = cache.try_emplace(market);
  return {iter->second, is_new};
}

core::BestQuotes& BestQuotesCache::set_quotes(core::MarketIdent market, core::BestQuotes const& quotes) {
  auto& item =  cache[market];
  item = quotes;
  return item;
}

} // namespace roq::core