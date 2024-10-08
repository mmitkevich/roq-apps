#pragma once
#include "roq/core/exposure.hpp"
#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/market.hpp"
//#include "roq/lqs/underlying.hpp"
#include <roq/parameter.hpp>
#include "roq/core/contract_style.hpp"
#include "roq/core/execution_mode.hpp"

namespace roq::lqs {

struct Pricer;
struct Underlying;
struct Strategy;

using LegIdent = uint32_t;


struct Leg {
  core::Market market;  
  core::MarketIdent underlying;
  
  core::ContractStyle contract_style;

  core::BestQuotes position {
    .buy = { .volume = 0},
    .sell = { .volume = 0}
  };
  core::Volume target_position = {0};
  core::ExecutionMode execution_mode = core::ExecutionMode::CROSS;
  //core::Volume exposure; // = position.buy.volume - position.sell.volume

  core::Double delta_greek = 1.0;         // of leg price to underlying price, e.g. 1
  core::BestQuotes exec_quotes;
  core::BestQuotes market_quotes;
  core::Volume buy_volume, sell_volume;
  core::Double slippage;
  core::Double delta_by_volume = 1.0;
  roq::Account account;

  core::Price tick_size = 0;
  // BLA-BLA

  core::Duration ban_fill;    // don't place new orders after order has been filled in full for at least post_filled_delay

  void operator()(const roq::Parameter& p, lqs::Strategy& s, std::string_view label);
  void operator()(const core::Exposure& e, lqs::Strategy& s);

  bool check_market_quotes(core::BestQuotes& m);
  void compute(lqs::Strategy const& strategy, lqs::Underlying const* underlying_opt=nullptr);
};

} // roq::lqs