#pragma once
#include "roq/core/types.hpp"
#include "roq/core/exposure.hpp"
#include "roq/side.hpp"
#include "roq/core/best_quotes.hpp"

namespace roq::lqs {


using LegIdent = uint32_t;

struct Leg {
  enum class Flags : uint32_t {
    LOCAL
  };

  roq::Mask<Flags> flags;
  core::MarketIdent market;
  roq::Side side {};  // side of the leg // parameter
  core::Double delta_by_volume {1.}; // from volume to delta  // parameter
  core::BestQuotes exec_quotes; // prices (P^B,P^S), volumes(V^B, V^s) // we should cacluate
  core::BestQuotes market_quotes; // price (M^B, M^S), volumes(U^B, U^S)  // given from market

  core::BestQuotes position;    // position now: buy.volume @ buy.price and sell.volume @ sell.price
  core::BestQuotes position_0;  // position when we were hedged
  core::BestQuotes fills;

  core::Volume exposure;  // N_l
  core::Double delta_buy {0.};
  core::Double delta_sell {0.};


  constexpr bool is_local_leg() const { return flags.has(Flags::LOCAL); }
  
  void operator()(core::Exposure const& u);

  void compute();
  void compute_delta();
  void compute_fills();
};


} // roq::lqs