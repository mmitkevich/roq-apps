// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/types.hpp"
#include "roq/core/quote.hpp"
#include "roq/core/best_quotes.hpp"
namespace roq::core {


struct Quotes  {
    core::MarketIdent market {};
    std::string_view symbol {};    
    std::string_view exchange {};
    std::string_view account {};
    core::PortfolioIdent portfolio {};
    core::StrategyIdent strategy {};    // FIXME: portfolios look like indexed by strategy id always.
    std::span<const ExecQuote> buy = {};
    std::span<const ExecQuote> sell = {};

    ExecQuote get_best_buy() const { return buy.empty() ? ExecQuote{} : buy[0]; }
    ExecQuote get_best_sell() const { return sell.empty() ? ExecQuote{} : sell[0]; }

    operator core::BestQuotes() const {
      return {
        .buy = get_best_buy(), 
        .sell = get_best_sell()
      };
    }
};

inline core::Quotes to_quotes(core::BestQuotes const& best_quotes, core::MarketIdent market) {
  return {
    .market = market,
    .buy = {&best_quotes.buy, 1},
    .sell = {&best_quotes.sell, 1}
  };
}


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
          "market.{} {}@{} buy_price {} sell_price {} buy_volume {} sell_volume {} account {} strategy.{} portfolio.{}"sv,
          _.market, _.symbol, _.exchange,
          buy_price, sell_price, buy_volume, sell_volume,
          _.account, _.strategy, _.portfolio);
    }
};