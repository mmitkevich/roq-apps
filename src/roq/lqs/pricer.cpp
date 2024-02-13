#include "roq/core/manager.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/quotes.hpp"
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

std::pair<lqs::Portfolio&, bool> Pricer::emplace_portfolio(std::string_view portfolio_name) {
    auto [portfolio_1, _] = core.portfolios.emplace_portfolio({.portfolio_name = portfolio_name});
    auto [iter, is_new] = portfolios.try_emplace(portfolio_1.portfolio, *this);
    lqs::Portfolio& portfolio = iter->second;
    return {portfolio, is_new};
}

void Pricer::operator()(const roq::Event<roq::ParametersUpdate> & e) {
    for(const auto& p: e.value.parameters) {
        log::debug("lqs parameter label {} exchange {} symbol {} portfolio {} value {}", p.label, p.exchange, p.symbol, p.account, p.value);
        core::MarketIdent market_id = core.markets.get_market_ident(p.symbol, p.exchange);
        auto [portfolio, is_new_portfolio] = emplace_portfolio(p.account);
        portfolio(p);
    }
}

void Pricer::operator()(const roq::Event<core::ExposureUpdate> &e) {
    const auto& u = e.value;
    get_portfolio(u.portfolio, [&](lqs::Portfolio& portfolio) {
        for(const auto& exposure: u.exposure) {
            auto [this_leg, is_new_leg] = portfolio.emplace_leg(exposure.symbol, exposure.exchange);
            this_leg(exposure, portfolio);
            portfolio.get_underlying(this_leg, [&](lqs::Underlying & underlying) {
                underlying.compute(portfolio);            
                this_leg.compute(underlying, portfolio);
                // all legs affected by change in delta
                portfolio.get_legs(underlying, [&](lqs::Leg& leg) {
                    leg.compute(underlying, portfolio);
                    dispatch(leg);
                });
            });
        }
    });
}

void Pricer::operator()(const roq::Event<core::Quotes> &e) {
    core::Quotes const& u = e.value;
    get_portfolios([&](lqs::Portfolio& portfolio) {
        // broadcast to all portfolios
        bool result = true;
        result &= portfolio.get_leg(u.market, [&](lqs::Leg & leg) { 
            result &= core.best_quotes.get_quotes(u.market, [&] (core::BestQuotes const& market_quotes) {
                leg.market_quotes = market_quotes;
            });
            result &= portfolio.get_underlying(leg, [&] (lqs::Underlying & underlying ) {
                underlying.compute(portfolio);
                leg.compute(underlying, portfolio);
                dispatch(leg);
            });
        });
    });
    // return result;
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
