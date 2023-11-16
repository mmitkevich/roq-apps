// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include <roq/core/types.hpp>
#include <roq/event.hpp>
#include <roq/core/best_price_source.hpp>
#include <roq/string_types.hpp>

namespace roq::core {

struct Market {
    core::MarketIdent market;
    std::string_view symbol;
    std::string_view exchange;
};

struct MarketInfo {
    core::MarketIdent market;
    std::string_view symbol;
    std::string_view exchange;

    operator core::Market() const {
        return core::Market {
            .market = market,
            .symbol = symbol,
            .exchange = exchange,
        };
    }

    // preferred
    uint32_t mdata_gateway_id = -1;
    roq::Source mdata_gateway_name;
    
    // preferred
    uint32_t trade_gateway_id = -1;
    roq::Source trade_gateway_name;

    core::BestPriceSource pub_price_source;
    core::BestPriceSource best_price_source = core::BestPriceSource::MARKET_BY_PRICE;
    
    core::Volume lot_size = 1;

    uint32_t depth_num_levels = 0;
};

} // roq::core

template <>
struct fmt::formatter<roq::core::Market> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(const roq::core::Market &value, Context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        "market {} symbol {} exchange {}"sv,
        value.market,
        value.symbol,
        value.exchange);
  }
};