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
    std::string_view account;
    uint32_t strategy_id {0};
};

} // roq::core

template <>
struct fmt::formatter<roq::core::Market> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(const roq::core::Market &_, Context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        "market {} symbol {} exchange {} account {} strategy_id {}"sv,
        _.market,
        _.symbol,
        _.exchange,
        _.account,
        _.strategy_id
        );
  }
};