#pragma once
#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/market.hpp"


namespace roq::lqs {


struct Leg {
  core::Market market;  
  core::MarketIdent underlying;
  core::BestQuotes position;
  core::Double delta = 1.0;         // of leg price to underlying price, e.g. 1
  core::BestQuotes exec_quotes;
  core::BestQuotes market_quotes;
  core::Volume buy_volume, sell_volume;
};

} // roq::lqs