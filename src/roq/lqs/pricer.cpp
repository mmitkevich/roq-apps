#include "roq/core/manager.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/quotes.hpp"
#include "roq/core/utils/string_utils.hpp"
#include "roq/lqs/pricer.hpp"
#include "roq/logging.hpp"
#include "roq/core/market/manager.hpp"
#include "roq/lqs/underlying.hpp"

namespace roq::lqs {

using namespace std::literals;

Pricer::Pricer(core::Dispatcher &dispatcher, core::Manager &core)
: dispatcher(dispatcher)
, core(core)
{}

std::pair<lqs::Underlying&, bool> Pricer::emplace_underlying(std::string_view symbol, std::string_view exchange) {
    auto [market, is_new_market] = core.markets.emplace_market(core::Market {
        .symbol = symbol,
        .exchange = exchange
    });
    auto [iter, is_new] = underlyings.try_emplace(market.market, lqs::Underlying {
        .market = market,
        .delta = 0
    });
    return {iter->second, is_new};
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
        log::debug("lqs parameter label {} exchange {} symbol {} value {}", p.label, p.exchange, p.symbol, p.value);
        core::MarketIdent id = core.markets.get_market_ident(p.symbol, p.exchange);
        if(p.label == "underlying"sv) {
            auto [leg, is_new_leg] = emplace_leg(p.symbol, p.exchange);
            auto [exchange, symbol] = core::utils::split_prefix(p.value, ':');
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
        } else if(p.label == "enabled"sv) {
            core::PortfolioIdent portfolio = core.portfolios.get_portfolio_ident(p.value);
            if(portfolio) {
                lqs_portfolios_[portfolio] = (std::string_view {p.value} == "true"sv);
                log::info("lqs portfolio {} flag {}", portfolio, lqs_portfolios_[portfolio]);
            } else {
                log::warn("lqs portfolio not found {}", p.value);
            }
        } else  if(p.exchange == lqs::EXCHANGE) {    // underlyings are identified by having 'lqs' exchange
            auto [underlying, is_new_underlying] = emplace_underlying(p.symbol, p.exchange);
            underlying(p, *this);
        } else { // otherwise it is leg
            auto [leg, is_new_leg] = emplace_leg(p.symbol, p.exchange);
            leg(p, *this);
        }
    }
}

void Pricer::operator()(const roq::Event<core::ExposureUpdate> &e) {
    const auto& u = e.value;
    for(const auto& exposure: u.exposure) {
        auto [leg, is_new_leg] = emplace_leg(exposure.symbol, exposure.exchange);
        leg(exposure, *this);
        get_underlying(leg, [&](lqs::Underlying & underlying) {
            underlying.compute(*this);            
            leg.compute(underlying, *this);
            // all legs affected by change in delta
            get_legs(underlying, [&](lqs::Leg& some_leg) {
                some_leg.compute(underlying, *this);
                dispatch(some_leg);
            });
        });

    }
}

void Pricer::operator()(const roq::Event<core::Quotes> &e) {
    core::Quotes const& u = e.value;
    // update leg cache
    bool result = true;
    result &= get_leg(u.market, [&](lqs::Leg & leg) { 
        result &= core.best_quotes.get_quotes(u.market, [&] (core::BestQuotes const& market_quotes) {
            leg.market_quotes = market_quotes;
        });
        result &= get_underlying(leg, [&] (lqs::Underlying & underlying ) {
            underlying.compute(*this);
            leg.compute(underlying, *this);
            dispatch(leg);
        });
    });
}


void Pricer::dispatch(lqs::Leg const& leg) {
    //roq::MessageInfo info{};
    core::TargetQuotes target_quotes {
        .market = leg.market.market,
        .symbol = leg.market.symbol,        
        .exchange = leg.market.exchange,        
        .account = leg.account,
        .buy = std::span { &leg.exec_quotes.buy, 1},
        .sell = std::span { &leg.exec_quotes.sell, 1},
    };
          
    dispatcher(target_quotes); // to OMS
}


} // namespace roq::spreader
