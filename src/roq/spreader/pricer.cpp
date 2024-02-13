#include "roq/core/manager.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/quotes.hpp"
#include "roq/spreader/pricer.hpp"
#include "roq/logging.hpp"

#include "roq/spreader/hedge.cpp"
#include "roq/spreader/bait.cpp"
#include <roq/string_types.hpp>

namespace roq::spreader {

Pricer::Pricer(core::Dispatcher &dispatcher, core::Manager &core)
: dispatcher(dispatcher)
, core(core)
{}


void Pricer::build_spreads() {
    core.portfolios.get_portfolios([&](core::Portfolio const& portfolio) {
        spreader::Spread& spread = spreads.emplace_back();
        core.portfolios.get_positions(portfolio.portfolio, [&](core::ExposureUpdate const& update) {
            for(core::Exposure const& exposure : update.exposure) {
                spread.get_legs([&](LegIdent leg_id, spreader::Leg & leg) {
                    leg = {
                        .market = { 
                            .market = exposure.market, 
                            .symbol = exposure.symbol,
                            .exchange = exposure.exchange,
                        },
                        .side = leg_id == 0 ? roq::Side::BUY : roq::Side::SELL,    
                        .position = {
                            .buy = {
                                .price = exposure.avg_price_buy,                            
                                .volume = exposure.position_buy,
                            },
                            .sell = {
                                .price = exposure.avg_price_sell,                            
                                .volume = exposure.position_sell,
                            }
                        }
                    };
                });
            }
        });
    });
}

void Pricer::operator()(const roq::Event<roq::ParametersUpdate> & e) {
    dispatch(e);
}

bool Pricer::get_leg(core::MarketIdent market, std::invocable<spreader::Spread &, LegIdent> auto fn) {
    auto iter = leg_by_market.find(market);
    if(iter == std::end(leg_by_market))
        return false;
    spreader::SpreadIdent spread_id = iter->second.first;
    spreader::LegIdent leg_id = iter->second.second;
    assert(spread_id<spreads.size());
    assert(leg_id<2);
    fn(spreads[spread_id], leg_id);
    return true;
}

void Pricer::operator()(const roq::Event<core::Quotes> &e) {
    core::Quotes const& u = e.value;
    // update leg cache
    bool result = true;
    result &= get_leg(u.market, [&](spreader::Spread & spread, spreader::LegIdent leg_id) { 
        spreader::Leg& leg = spread.get_leg(leg_id);

         result &= core.best_quotes.get_quotes(u.market, [&] (core::BestQuotes const& market_quotes) {
            leg.market_quotes = market_quotes;
        });

        result &= baiter(spread, [&](spreader::Leg const& bait_leg) {
            log::debug<2>("bait_leg.exec_quotes {}", bait_leg.exec_quotes);
            dispatch(bait_leg);
        });

        result &= hedger(spread, [&](spreader::Leg const& hedge_leg) {
            log::debug<2>("hedge_leg.exec_quotes {}", hedge_leg.exec_quotes);
            dispatch(hedge_leg);
        });
    });
    
}

void Pricer::dispatch(spreader::Leg const& leg) {
    roq::MessageInfo info{};
    roq::Event event{info, core::to_quotes(leg.exec_quotes, leg.market.market)};
    dispatcher(event); // to OMS
}


} // namespace roq::spreader
