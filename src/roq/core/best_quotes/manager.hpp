#pragma once
#include "roq/core/best_quotes.hpp"
#include "roq/core/types.hpp"
#include "roq/event.hpp"
#include "roq/core/hash.hpp"

namespace roq::core::best_quotes {

struct Manager {
    bool get_quotes(core::MarketIdent market, std::invocable<core::BestQuotes const&> auto fn) {
        auto iter = cache.find(market);
        if(iter==std::end(cache))
            return false;
        core::BestQuotes const& best_quotes = iter->second;
        fn(best_quotes);
        return true;
    }

    std::pair<core::BestQuotes &, bool> emplace_quotes(core::MarketIdent market);
    core::BestQuotes& set_quotes(core::MarketIdent market, core::BestQuotes const& quotes);

    core::Hash<MarketIdent, core::BestQuotes> cache;
};

} // roq::cache