#include "roq/pricer/compute.hpp"
#include "roq/pricer/ops/quote_spread.hpp"
#include "roq/core/price.hpp"


namespace roq::pricer::ops {

bool QuoteSpread::operator()(pricer::Context &context) const {
  auto& params = context.get_parameters<Parameters>();

  core::BestQuotes &q = context.quotes;
  core::Price mid_price = core::MidPrice(q);

  if (core::is_empty_value(mid_price)) {
    context.quotes.clear();
  }
  auto [bid_price, ask_price] = core::Spread(params.min_spread, mid_price);
  return true;
}

bool QuoteSpread::operator()(pricer::Context &context, std::span<const roq::Parameter>  update)  {
  using namespace std::literals;
  auto& parameters = context.template get_parameters<Parameters>();
  for(auto const& p: update) {
    if (p.label == "quote_spread:min_spread"sv) {
      parameters.min_spread = core::Shift::parse(p.value);
    } else if (p.label == "quote_spread:max_spread"sv) {
      parameters.max_spread = core::Shift::parse(p.value);
    }
  }
  return false;
}

} // namespace roq::pricer::ops