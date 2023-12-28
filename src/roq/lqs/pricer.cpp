#include "roq/core/manager.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/quotes.hpp"
#include "roq/lqs/pricer.hpp"
#include "roq/logging.hpp"

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
                .side = exposure.side,
            //    .volume = 0,
            };
        });
    });
}

bool Pricer::compute(core::MarketIdent market, std::invocable<core::Quotes const&> auto fn) {
    bool result = true;
    
    result &= get_leg(market, [&](lqs::Spread & spread, lqs::LegIdent leg_id) {
        lqs::Leg& leg = spread.get_leg(leg_id);

        // cache prices (used to compare legs)
        result &= core.best_quotes.get_quotes(market, [&] (core::BestQuotes const& market_quotes) {
            leg.market_quotes = market_quotes;
        });

        lqs::Leg& hedge_leg = spread.get_hedge_leg(leg_id);
        
        // given current delta...
        core::Double delta = 0; // FIXME =sum(leg.position*leg.delta_by_volume)
        
        // ... we're going to target_delta (e.g, zero) ...

        // ... and first (BUY) leg exposure...
        core::Double target_exposure_buy = 0; // of BUY leg in terms of delta

        core::Double target_delta = 0;

        // ... what gives us desired sell exposure as well
        core::Double target_exposure_sell = target_delta - target_exposure_buy;

        // ... we get buy_delta to buy in first BUY leg OR sell in second SELL leg to have ZERO delta
        core::Double buy_delta = std::max(target_delta-delta, ZERO);

        // ... and sell_delta to sell in first BUY leg OR buy in second SELL leg to have ZERO delta - 
        core::Double sell_delta = std::max(delta-target_delta, ZERO);

        assert(buy_delta==0 || sell_delta==0);

        // ... now we translate delta into leg's volume...
        if(leg.side==Side::BUY) {
            leg.exec_quotes.buy.volume = buy_delta / leg.delta_by_volume;
            leg.exec_quotes.sell.volume = sell_delta / leg.delta_by_volume;

        } else if (leg.side==Side::SELL) {
            leg.exec_quotes.buy.volume = sell_delta / leg.delta_by_volume;
            leg.exec_quotes.sell.volume = buy_delta / leg.delta_by_volume;      
        }

        // now exec_quotes.(buy|sell).volume are all set up to make delta == target_delta (hedge)

        // n

        // given current exposure (in delta scale)...
        core::Double exposure = leg.position * leg.delta_by_volume;
        // we could exclude increasing delta
        if(leg.side==Side::BUY) {    
            // ... make sure that we're moving towards target_exposure_buy
            leg.exec_quotes.buy.volume = std::min(leg.exec_quotes.buy.volume, std::max(target_exposure_buy-exposure, ZERO)/leg.delta_by_volume);
            leg.exec_quotes.sell.volume = std::min(leg.exec_quotes.sell.volume, std::max(exposure-target_exposure_buy, ZERO)/leg.delta_by_volume);

        } else if (leg.side==Side::SELL) {            
            // ... make sure that we're moving towards target_exposure_sell
            leg.exec_quotes.buy.volume = std::min(leg.exec_quotes.buy.volume, std::max(target_exposure_sell-exposure, ZERO)/leg.delta_by_volume);
            leg.exec_quotes.sell.volume = std::min(leg.exec_quotes.sell.volume, std::max(exposure-target_exposure_sell, ZERO)/leg.delta_by_volume);
        }

        // ... and ensure that we could hedge as fast as possible ...
        leg.exec_quotes.buy.volume = std::min(leg.exec_quotes.buy.volume, hedge_leg.market_quotes.sell.volume);
        leg.exec_quotes.sell.volume = std::min(leg.exec_quotes.sell.volume, hedge_leg.market_quotes.buy.volume);
    });
    
    result &= core.best_quotes.get_quotes(market, [&] (core::BestQuotes const& market_quotes) {
        // take market BBO prices
        core::BestQuotes exec_quotes = market_quotes;   // for 'market'
        // spread them little bit
        exec_quotes.buy.price /= 1.1;
        exec_quotes.sell.price *= 1.1;
        exec_quotes.buy.volume = exec_quotes.sell.volume = 0;
        
        // here portfolio is a liquidation group

        // send out TargetQuotes
        fn(core::to_quotes(exec_quotes, market));
    });
    return result;
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
    compute(e.value.market, [&](core::Quotes const& exec_quotes) {
        log::debug<2>("lqs::Quotes market_quotes {} exec_quotes {}", e.value, exec_quotes);
        roq::MessageInfo info{};
        roq::Event event{info, exec_quotes};
        dispatcher(event); // send to OMS
    });
}

} // namespace roq::lqs
