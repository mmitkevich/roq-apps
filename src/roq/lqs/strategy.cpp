#include "roq/lqs/strategy.hpp"
#include "roq/lqs/pricer.hpp"
#include "roq/core/string_utils.hpp"

namespace roq::lqs {
using namespace std::literals;

std::pair<lqs::Underlying&, bool> Strategy::emplace_underlying(std::string_view symbol, std::string_view exchange) {
    auto [market, is_new_market] = pricer.core.markets.emplace_market(core::Market {
        .symbol = symbol,
        .exchange = exchange
    });
    auto [iter, is_new] = underlyings.try_emplace(market.market, lqs::Underlying {
        .market = market,
        .delta = 0
    });
    return {iter->second, is_new};
}
  
std::pair<lqs::Leg&, bool> Strategy::emplace_leg(std::string_view symbol, std::string_view exchange) {
    auto [market, is_new_market] = pricer.core.markets.emplace_market(core::Market {.symbol=symbol, .exchange=exchange});
    auto [iter, is_new_leg] = leg_by_market.try_emplace(market.market, lqs::Leg {
        .market = {
            .market = market.market,
            .symbol = market.symbol,
            .exchange = market.exchange
        },
        .underlying = static_cast<core::MarketIdent>(-1),
    });
    return {iter->second, is_new_leg};
}

void Strategy::operator()(roq::Parameter const & p) {
    auto [prefix, label] = core::split_prefix(p.label, ':');
    if(label == "underlying"sv) {
        auto [leg, is_new_leg] = emplace_leg(p.symbol, p.exchange);
        auto [exchange, symbol] = core::split_prefix(p.value, ':');
        auto [underlying, is_new_underlying] = emplace_underlying(symbol, lqs::EXCHANGE);
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
        auto [underlying, is_new_underlying] = emplace_underlying(p.symbol, p.exchange);
        underlying(p, *this);
    } else { // otherwise it is leg
        auto [leg, is_new_leg] = emplace_leg(p.symbol, p.exchange);
        leg(p, *this);
    }
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

} // roq::lqs