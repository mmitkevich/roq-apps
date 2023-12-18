#pragma once
#include "roq/core/best_quotes.hpp"
#include "roq/core/types.hpp"
namespace roq::core {

struct BestQuotesCache {

    void operator()(Event<Quotes> const& quotes);

    bool get_quotes(core::MarketIdent market, std::invocable<core::BestQuotes const&> auto fn) {
        auto iter = cache.find(market);
        if(iter==std::end(cache))
            return false;
        core::BestQuotes const& best_quotes = iter->second;
        fn(best_quotes);
        return true;
    }

    std::pair<core::BestQuotes &, bool> emplace_quotes(core::BestQuotes const &quotes);

    core::Hash<MarketIdent, core::BestQuotes> cache;
};

} // roq::cache