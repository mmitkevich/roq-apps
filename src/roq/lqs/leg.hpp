#pragma once
#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/market.hpp"
#include "roq/lqs/underlying.hpp"


namespace roq::lqs {


struct Leg {
  core::Market market;  
  core::MarketIdent underlying;
  core::BestQuotes position;
  
  core::Volume exposure; // = position.buy.volume - position.sell.volume

  core::Double delta_greek = 1.0;         // of leg price to underlying price, e.g. 1
  core::BestQuotes exec_quotes;
  core::BestQuotes market_quotes;
  core::Volume buy_volume, sell_volume;
  core::Double slippage;
  core::Double delta_by_volume;

  core::Duration ban_fill;    // don't place new orders after order has been filled in full for at least post_filled_delay

  void compute(lqs::Underlying const& underlying, lqs::Pricer& pricer);
};

} // roq::lqs