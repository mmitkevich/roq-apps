#pragma once
#include "roq/core/market.hpp"
#include "roq/core/types.hpp"
#include "roq/spreader/leg.hpp"
#include <roq/parameter.hpp>
#include <roq/parameters_update.hpp>

namespace roq::spreader {

using SpreadIdent = uint32_t;
// buy_1 = bid_2 * M
// sell_1 = ask_2 * M
// buy_2 = bid_1 / M
// sell_2 = ask_1 / M

struct Pricer;

struct Spread {
    std::array<spreader::Leg,2> legs;
    core::PortfolioIdent portfolio;   // identifier of liquidation group
  
    core::Market market;           // identifier of spread as virtual market (symbol='D:BTC.1/D:BTC.P', exchange='lqs')

    core::BestQuotes exec_quotes;     // pair of limit orders where we want to buy the spread as a whole and where we want to sell it.
    // exec_quotes.buy.volume = exec_quots.sell.volume = d
    // exec_quotes.buy.price = L^B; exec_quotes.sell.price = L^S
  
    core::BestQuotes market_quotes;   // informative price of the spread as a combination, calculated, price of the spread liquidation into the market

    core::Range<core::Double> delta_range;
    core::Double delta;
    
    core::TimePoint last_hedged_time;

    
    void operator()(const roq::Event<roq::ParametersUpdate> & e, spreader::Pricer& ctx);

    constexpr Leg& get_leg(LegIdent leg_id) {
        assert(leg_id<legs.size());
        return legs[leg_id];
    }

    constexpr void get_legs(std::invocable<LegIdent, spreader::Leg &> auto fn) {
        for(LegIdent leg_id=0; leg_id<legs.size(); leg_id++) {
        fn(leg_id, get_leg(leg_id));
        }
    }
    
    constexpr LegIdent get_other_leg_ident(LegIdent leg_id) {
        assert(leg_id<2);
        return 1-leg_id;
    }

    constexpr Leg& get_other_leg(LegIdent leg_id) {
        return legs[get_other_leg_ident(leg_id)];
    }
    
    void compute();
    
    void compute_delta();
};


} // roq::spreader