#include "roq/lqs/strategy.hpp"
#include "roq/lqs/pricer.hpp"
#include "roq/core/string_utils.hpp"

namespace roq::lqs {
using namespace std::literals;

std::pair<lqs::Underlying&, bool> Strategy::emplace_underlying(core::Market const& key) {
    auto [info, is_new_info] = pricer.core.markets.emplace_market(key);

    auto [iter, is_new] = underlyings.try_emplace(info.market, lqs::Underlying {
        .market = info,
        .delta = 0
    });

    return {iter->second, is_new};
}
  
std::pair<lqs::Leg&, bool> Strategy::emplace_leg(core::Market const& key) {
    auto [info, is_new_info] = pricer.core.markets.emplace_market(key);
    auto [iter, is_new] = leg_by_market.try_emplace(key.market, lqs::Leg {
        .market = {
            .market = info.market,
            .symbol = info.symbol,
            .exchange = info.exchange
        },
        .underlying = static_cast<core::MarketIdent>(-1),
    });
    return {iter->second, is_new};
}

bool Strategy::operator()(roq::Parameter const & p) {
    bool result = true;

    auto [prefix, label] = core::split_prefix(p.label, ':');
    core::Market key {
        .symbol = p.symbol,
        .exchange = p.exchange
    };
    if(label == "underlying"sv) {
        auto [leg, is_new_leg] = emplace_leg(key);
        auto [exchange, symbol] = core::split_prefix(p.value, ':');
        auto [underlying, is_new_underlying] = emplace_underlying(core::Market{
            .symbol = symbol,
            .exchange = lqs::EXCHANGE
        });
        if(leg.underlying) {
            auto iter = underlyings.find(leg.underlying);
            if(iter!=std::end(underlyings)) {
                auto& prev_underlying = iter->second;
                prev_underlying.remove_leg(leg.market.market);
                log::debug("lqs remove leg {} from underlying {}", leg.market.market, underlying.market.market);
            }
        }
        leg.underlying = underlying.market.market;
        underlying.legs.push_back(leg.market.market);
        log::debug("lqs add leg {} to underlying {}", leg.market.market, underlying.market.market);
    } else if(label == "enabled"sv) {
        enabled = (std::string_view {p.value} == "true"sv);
    } else if(p.exchange == lqs::EXCHANGE) {    // underlyings are identified by having 'lqs' exchange
        auto [underlying, is_new_underlying] = emplace_underlying(key);
        underlying(p, *this);
    } else { // otherwise it is leg
        if(!p.symbol.empty()) {
            // concrete leg - may emplace
            assert(!p.exchange.empty());
            auto [leg, is_new_leg] = emplace_leg(key);
            leg(p, *this);
        } else {
            // broadcast to all legs
            get_legs([&](lqs::Leg& leg) {
                leg(p, *this);
            });
        }
    }
    return result;
}


bool Strategy::compute(lqs::Leg& this_leg) {
    if(!get_underlying(this_leg, [&](lqs::Underlying& u) {
        // has underlying
        u.compute(*this);            
        this_leg.compute(*this, &u);
        // all legs affected by change in delta
        get_legs(u, [&](lqs::Leg& leg) {
            leg.compute(*this, &u);
            pricer.dispatch(leg, *this);
        });
        return true;
    })) {
        // no underlying
        this_leg.compute(*this);
        pricer.dispatch(this_leg, *this);
        return true;
    }
    return false;
}

bool Strategy::operator()(core::Quotes const& u) {
    bool result = true;

    auto fn = [&](lqs::Leg & this_leg) {
        auto& best_quotes = pricer.core.best_quotes;
        result &= best_quotes.get_quotes(u.market, [&] (core::BestQuotes const& market_quotes) {
            this_leg.market_quotes = market_quotes;
        });
        // delegate to the strategy to compute things after this_leg changed
        result &= compute(this_leg);
    };
    if(auto_legs) {
        auto [leg, is_new] = emplace_leg(core::Market {
            .market = u.market,
            .symbol = u.symbol,
            .exchange = u.exchange,
        });
        fn(leg);
    } else {
        result &= get_leg(u.market, std::move(fn));
    }
    return result;
}

bool Strategy::operator()(core::Exposure const& e) {
    core::Market key {
        .market = e.market,
        .symbol = e.symbol,
        .exchange = e.exchange,
    };
    auto [this_leg, is_new_leg] = emplace_leg(key);
    this_leg(e, *this);
    compute(this_leg);
    return true;
}

} // roq::lqs