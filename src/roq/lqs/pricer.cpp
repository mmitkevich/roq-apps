#include "roq/core/manager.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/quotes.hpp"
#include "roq/lqs/pricer.hpp"
#include "roq/logging.hpp"

#include "roq/lqs/hedge.cpp"
#include "roq/lqs/bait.cpp"

namespace roq::lqs {

Pricer::Pricer(core::Dispatcher &dispatcher, core::Manager &core)
: dispatcher(dispatcher)
, core(core)
{}


void Pricer::build_spreads() {
    core.portfolios.get_portfolios([&](core::Portfolio const& portfolio) {
        lqs::Spread& spread = spreads.emplace_back();
        core.portfolios.get_exposures(portfolio.portfolio, [&](core::Exposure const& exposure) {
            uint32_t leg_id = (exposure.side == Side::BUY) ? 0 : 1;
            spread.legs[leg_id] = lqs::Leg { 
                .market = exposure.market,
                .side = exposure.side,
            };
        });
    });
}

void Pricer::operator()(const roq::Event<roq::ParameterUpdate> & e) override {
    const auto& u = e.value;
    for(roq::Parameter const & p: u.parameters) {
        
    }
}


bool Pricer::get_leg(core::MarketIdent market, std::invocable<lqs::Spread &, LegIdent> auto fn) {
    auto iter = leg_by_market.find(market);
    if(iter == std::end(leg_by_market))
        return false;
    lqs::SpreadIdent spread_id = iter->second.first;
    lqs::LegIdent leg_id = iter->second.second;
    assert(spread_id<spreads.size());
    assert(leg_id<2);
    fn(spreads[spread_id], leg_id);
    return true;
}

void Pricer::operator()(const roq::Event<core::Quotes> &e) {
    core::Quotes const& u = e.value;
    // update leg cache
    bool result = true;
    result &= get_leg(u.market, [&](lqs::Spread & spread, lqs::LegIdent leg_id) { 
        lqs::Leg& leg = spread.get_leg(leg_id);

         result &= core.best_quotes.get_quotes(u.market, [&] (core::BestQuotes const& market_quotes) {
            leg.market_quotes = market_quotes;
        });

        result &= baiter(spread, [&](lqs::Leg const& bait_leg) {
            log::debug<2>("bait_leg.exec_quotes {}", bait_leg.exec_quotes);
            dispatch(bait_leg);
        });

        result &= hedger(spread, [&](lqs::Leg const& hedge_leg) {
            log::debug<2>("hedge_leg.exec_quotes {}", hedge_leg.exec_quotes);
            dispatch(hedge_leg);
        });
    });
    
}

void Pricer::dispatch(lqs::Leg const& leg) {
    roq::MessageInfo info{};
    roq::Event event{info, core::to_quotes(leg.exec_quotes, leg.market)};
    dispatcher(event); // to OMS
}


} // namespace roq::lqs
