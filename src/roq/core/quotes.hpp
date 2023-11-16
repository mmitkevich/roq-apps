// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/types.hpp"
#include "roq/core/quote.hpp"

namespace roq::core {


struct Quotes  {
    core::MarketIdent market {};
    std::string_view symbol {};    
    std::string_view exchange {};
    std::string_view account {};
    std::string_view portfolio {};
    std::span<const Quote> buy = {};
    std::span<const Quote> sell = {};
};

// bids = [ {19000, 100=50+50} , {19100,200} ], asks = [ {20100, 200}, {20200, 250}]

using TargetQuotes = core::Quotes;


} // roq::core


template <> 
struct fmt::formatter<roq::core::Quotes> {
    template <typename Context> 
    constexpr auto parse(Context &context) {
      return std ::begin(context);
    }
    template <typename Context>
    auto format(const roq::core::Quotes &_, Context &context) const {
      using namespace std ::literals;
      using namespace roq::core;
      Price buy_price, sell_price;
      Volume buy_volume, sell_volume;
      if(!std::empty(_.buy)) {
        buy_price = _.buy[0].price;
        buy_volume = _.buy[0].volume;
      }
      if(!std::empty(_.sell)) {
        sell_price = _.sell[0].price;
        sell_volume = _.sell[0].volume;
      }
      return fmt ::format_to(
          context.out(),
          "market {} buy_price {} sell_price {} buy_volume {} sell_volume {}"sv,
          _.market,
          buy_price,sell_price,buy_volume,sell_volume);
    }
};