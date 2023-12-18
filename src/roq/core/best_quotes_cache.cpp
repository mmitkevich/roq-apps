#include "roq/core/quotes_cache.hpp"
#include "roq/core/quotes.hpp"

namespace roq::core {

void BestQuotesCache::operator()(Event<Quotes> const& e) {
  core::BestQuotes const& best_quotes = e.value;
  auto [result, is_new] = emplace_quotes(best_quotes); 
}

std::pair<core::BestQuotes &, bool> BestQuotesCache::emplace_quotes(core::BestQuotes const &quotes) {
  auto [iter, is_new] = cache.try_emplace(quotes.market);
  auto &best_quotes = iter->second;
  best_quotes = quotes;
  return {iter->second, is_new};
}
} // namespace roq::core