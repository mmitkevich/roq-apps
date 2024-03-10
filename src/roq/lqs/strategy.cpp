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
    auto [iter, is_new] = leg_by_market.try_emplace(info.market, lqs::Leg {
        .market = {
            .market = info.market,
            .symbol = info.symbol,
            .exchange = info.exchange
        },
        .underlying = static_cast<core::MarketIdent>(-1),
    });
    lqs::Leg &leg = iter->second;
    leg.tick_size = info.tick_size;    
    if(is_new) {
        log::debug("lqs emplace_leg {} portfolio {} tick_size {}", leg.market, portfolio, leg.tick_size);
    }
    return {leg, is_new};
}

bool Strategy::operator()(roq::Parameter const & p, std::string_view label) {
    bool result = true;

    //auto [prefix, label] = core::split_prefix(p.label, ':');
    auto [prefix, sublabel] = core::split_prefix(label, ':');

    core::Market key {
        .symbol = p.symbol,
        .exchange = p.exchange
    };
    if(label == "underlying"sv) {
        auto [leg, is_new_leg] = emplace_leg(key);
        auto [exchange, symbol] = core::split_prefix(p.value, ':');
        auto [underlying, is_new_underlying] = emplace_underlying(core::Market{
            .symbol = symbol,
            .exchange = exchange
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
    } else if(prefix == "underlying"sv) {    // underlyings are identified by having 'lqs' exchange
        auto [underlying, is_new_underlying] = emplace_underlying(key);
        underlying(p, *this, sublabel);
    } else { // otherwise it is leg
        if(!p.symbol.empty() && !p.exchange.empty()) {
            // concrete leg - may emplace
            assert(!p.exchange.empty());
            auto [leg, is_new_leg] = emplace_leg(key);
            leg(p, *this, label);
            compute(leg);
        } else {
            // broadcast to all legs
            get_legs([&](lqs::Leg& leg) {
                leg(p, *this, label);
                compute(leg);
            });
        }
    }
    return result;
}

bool Strategy::operator()(roq::ReferenceData const& u) {
    core::Market key {
        .symbol = u.symbol,
        .exchange = u.exchange
    };
    bool found = true;
    found &= pricer.core.markets.get_market(key, [&](core::market::Info const& info) {
        assert(info.market);
        found &= get_leg(info.market, [&](lqs::Leg& leg) {
            leg.tick_size = u.tick_size;
            leg.tick_size = info.tick_size;    
            log::debug("lqs reference_data market {} tick_size {}", leg.market, leg.tick_size);
        });        
    });
    return found;
}
  


bool Strategy::compute(lqs::Leg& this_leg) {
    if(!get_underlying(this_leg, [&](lqs::Underlying& u) {
        // has underlying
        u.compute(*this);            
        //FIXME: this_leg.compute(*this, &u);
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
    bool result = false;
    auto& best_quotes = pricer.core.best_quotes;

    result |= get_leg(u.market, [&](lqs::Leg & this_leg) {
        result &= best_quotes.get_quotes(u.market, [&] (core::BestQuotes const& market_quotes) {
            this_leg.market_quotes = market_quotes;            
            log::debug("lqs quotes leg {} market_quotes {}", this_leg.market, this_leg.market_quotes);
        });
        // delegate to the strategy to compute things after this_leg changed
        result &= compute(this_leg);
    });

    result |= get_underlying(u.market, [&](lqs::Underlying & underlying) {
        result &= best_quotes.get_quotes(u.market, [&] (core::BestQuotes const& market_quotes) {
            underlying.market_quotes = market_quotes;            
            log::debug("lqs quotes underlying {} market_quotes {}", underlying.market, underlying.market_quotes);
        });
    });

    return result;
}

bool Strategy::operator()(core::Exposure const& e) {
    core::Market key {
        .market = e.market,
        .symbol = e.symbol,
        .exchange = e.exchange,
    };
    auto [this_leg, is_new_leg] = emplace_leg(key);
    if(is_new_leg) {
        assert(!e.account.empty());
        this_leg.account = e.account;        
        log::debug("emplace_leg market.{} {}@{} account {}", this_leg.market.market, this_leg.market.symbol, this_leg.market.exchange, this_leg.account);
    }
    this_leg(e, *this);
    compute(this_leg);
    return true;
}

bool Strategy::operator()(roq::DownloadBegin const& u) {
    return true;
}

bool Strategy::operator()(roq::DownloadEnd const& u) {
    if(!u.account.empty())
        return false;
    // only process MD-related DownloadEnd (s)
    get_legs([&](lqs::Leg& leg) {
        auto [info, is_new_info] = pricer.core.markets.emplace_market(leg.market);
        leg.tick_size = info.tick_size;    
        log::debug("lqs download_end market {} tick_size {}", leg.market, leg.tick_size);
        compute(leg);
    });
    return true;
}

} // roq::lqs