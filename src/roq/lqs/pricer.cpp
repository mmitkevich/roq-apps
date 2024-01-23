#include "roq/core/manager.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/quotes.hpp"
#include "roq/lqs/pricer.hpp"
#include "roq/logging.hpp"
#include "roq/core/markets.hpp"
#include "roq/lqs/underlying.hpp"

namespace roq::lqs {

using namespace std::literals;

Pricer::Pricer(core::Dispatcher &dispatcher, core::Manager &core)
: dispatcher(dispatcher)
, core(core)
{}

std::pair<lqs::Underlying&,bool> Pricer::emplace_underlying(std::string_view symbol) {
    auto iter = this->underlying_by_symbol.find(symbol);
    if(iter==std::end(underlying_by_symbol)) {
        lqs::Underlying& u = this->underlyings.emplace_back( lqs::Underlying {
            .market = static_cast<core::MarketIdent>(underlyings.size()),
            .symbol = symbol,
            .delta = 0
        });
        return {u, true};
    }
    return {this->underlyings[iter->second], false};
}
  
std::pair<lqs::Leg&, bool> Pricer::emplace_leg(std::string_view symbol, std::string_view exchange) {
    auto [market, is_new_market] = core.markets.emplace_market(core::Market {.symbol=symbol, .exchange=exchange});
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

void Pricer::operator()(const roq::Event<roq::ParametersUpdate> & e) {
    for(const auto& p: e.value.parameters) {
        if(p.label == "underlying"sv) {
            auto [leg, is_new_leg] = emplace_leg(p.symbol, p.exchange);
            auto [underlying, is_new_underlying] = emplace_underlying(p.value);
            leg.underlying = underlying.market.market;
        } else if(p.label == "delta"sv) {
            auto [leg, is_new_leg] = emplace_leg(p.symbol, p.exchange);
            leg.delta = core::Double::parse(p.value);
        }
    }
}

bool Pricer::get_leg(core::MarketIdent market, std::invocable<lqs::Leg &> auto fn) {
    auto iter = leg_by_market.find(market);
    if(iter == std::end(leg_by_market))
        return false;
    fn(iter->second);
    return true;
}

bool Pricer::get_underlying(lqs::Leg& leg, std::invocable<lqs::Underlying &> auto fn) {
    fn(underlyings[leg.underlying]);
    return true;
}

void Pricer::operator()(const roq::Event<core::Quotes> &e) {
    core::Quotes const& u = e.value;
    // update leg cache
    bool result = true;
    result &= get_leg(u.market, [&](lqs::Leg & leg) { 
        
        result &= core.best_quotes.get_quotes(u.market, [&] (core::BestQuotes const& market_quotes) {
            leg.market_quotes = market_quotes;
        });
        
        result &= get_underlying(leg, [&] (lqs::Underlying const& underlying ) {
            core::BestQuotes& q = leg.exec_quotes;
            core::BestQuotes& m = leg.market_quotes;
            q.buy.price = m.buy.price;
            q.buy.volume = leg.buy_volume;
            dispatch(leg);
        });
    });
    
}

void Pricer::dispatch(lqs::Leg const& leg) {
    roq::MessageInfo info{};
    roq::Event event{info, core::to_quotes(leg.exec_quotes, leg.market.market)};
    dispatcher(event); // to OMS
}


} // namespace roq::spreader
