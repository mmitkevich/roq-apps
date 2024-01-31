// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include <roq/core/types.hpp>
#include <roq/event.hpp>
#include <roq/core/best_quotes_source.hpp>
#include <roq/string_types.hpp>

namespace roq::core {

struct Market {
    core::MarketIdent market;
    std::string_view symbol;
    std::string_view exchange;
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