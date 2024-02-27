#include "roq/core/manager.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/quotes.hpp"
#include "roq/lqs/pricer.hpp"
#include "roq/logging.hpp"
#include "roq/core/market/manager.hpp"
#include "roq/lqs/underlying.hpp"
#include "roq/core/string_utils.hpp"
namespace roq::lqs {

using namespace std::literals;

Pricer::Pricer(core::Dispatcher &dispatcher, core::Manager &core)
: dispatcher(dispatcher)
, core(core)
{}

std::pair<lqs::Strategy&, bool> Pricer::emplace_strategy(core::StrategyIdent strategy_id) {
    //auto [portfolio, is_new_portfolio] = core.portfolios.emplace_portfolio({.portfolio_name = portfolio_name});
    auto [iter, is_new] = strategies_.try_emplace(strategy_id, *this);
    lqs::Strategy& strategy = iter->second;
    // NOTE: portfolio id == strategy id
    strategy.strategy = strategy_id;
    strategy.portfolio = strategy_id;
    if(is_new) {
        log::debug("lqs emplace_strategy {} is_new {}", strategy_id, is_new);
    }
    return {strategy, is_new};
}

void Pricer::operator()(const roq::Event<roq::ParametersUpdate> & e) {
    for(const roq::Parameter& p: e.value.parameters) {
        auto [prefix, label] = core::split_prefix(p.label,':');
        if(prefix!="lqs"sv)
            continue;
        log::debug("lqs parameters_update label {} exchange {} symbol {} portfolio {} strategy {} value {}", 
                                p.label, p.exchange, p.symbol, p.account, p.strategy_id, p.value);
        if(p.strategy_id) {
            auto [strategy, is_new] = emplace_strategy(p.strategy_id);
            strategy(p, label);
        } else {
            // broadcast to all the strategeis
            get_strategies([&](lqs::Strategy& strategy) {
                strategy(p, label);
            });
        }
    }
}

void Pricer::operator()(const roq::Event<core::ExposureUpdate> &event) {
    const auto& u = event.value;
    core::StrategyIdent strategy_id = u.portfolio;  // NOTE: portfolio id IS strategy id
    get_strategy(strategy_id, [&](lqs::Strategy& s) {
        for(const core::Exposure& e: u.exposure) {
            s(e);
        }
    });
}

void Pricer::operator()(const roq::Event<core::Quotes> &e) {
    const core::Quotes& u = e.value;
    get_strategies([&](lqs::Strategy& s) {
        // broadcast to all strategies
        s(u);
    });
    // return result;
}

// called by strategy when leg changed
void Pricer::dispatch(lqs::Leg const& leg, lqs::Strategy const& strategy) {
    //roq::MessageInfo info{};
    core::TargetQuotes target_quotes {
        .market = leg.market.market,
        .symbol = leg.market.symbol,        
        .exchange = leg.market.exchange,        
        .account = leg.account,
        .portfolio = strategy.portfolio,        
        .strategy = strategy.strategy,
        .buy = std::span { &leg.exec_quotes.buy, 1},
        .sell = std::span { &leg.exec_quotes.sell, 1},
    };
          
    dispatcher(target_quotes); // to OMS
}


} // namespace roq::spreader
