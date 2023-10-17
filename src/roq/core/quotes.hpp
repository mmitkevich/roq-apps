#pragma once

#include "roq/core/types.hpp"
#include "roq/core/quote.hpp"

namespace roq::core {


struct Quotes  {
    core::MarketIdent market {};
    std::string_view account {};
    std::string_view exchange {};
    std::string_view symbol {};
    std::string_view portfolio {};
    std::span<const Quote> bids = {};
    std::span<const Quote> asks = {};
};

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
      Price bid_price, ask_price;
      Volume bid_volume, ask_volume;
      if(!std::empty(_.bids)) {
        bid_price = _.bids[0].price;
        bid_volume = _.bids[0].volume;
      }
      if(!std::empty(_.asks)) {
        ask_price = _.asks[0].price;
        ask_volume = _.asks[0].volume;
      }
      return fmt ::format_to(
          context.out(),
          "market {} bid_price {} ask_price {} bid_volume {} ask_volume {}"sv,
          _.market,
          bid_price,ask_price,bid_volume,ask_volume);
    }
};