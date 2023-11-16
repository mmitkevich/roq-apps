#include "roq/pricer/compute.hpp"
#include "roq/pricer/ops/quote_spread.hpp"
#include "roq/pricer/ops/mid_price.hpp"

namespace roq::pricer::ops {

bool QuoteSpread::operator()(pricer::Context &context) const {
  auto& params = context.get_parameters<Parameters>();

  core::BestQuotes &q = context.quotes;
  core::Price mid_price = ops::MidPrice(q);

  if (core::is_empty_value(mid_price)) {
    context.quotes.clear();
  }

  switch (params.min_spread.units) {
    case core::Units::BP: {
        q.buy.price = mid_price / (1. + .5 * params.min_spread * QuoteSpread::BP);
        q.sell.price = mid_price * (1. + .5 * params.min_spread * QuoteSpread::BP);
    } break;
    case core::Units::PRICE: {
        q.buy.price = mid_price - 0.5 * params.min_spread;
        q.sell.price = mid_price + 0.5 * params.min_spread;
    } break;
    default: {
        throw roq::RuntimeError("UNEXPECTED");
    }
  }
  return true;
}

bool QuoteSpread::operator()(pricer::Context &context, std::span<const roq::Parameter>  update)  {
  using namespace std::literals;
  auto& parameters = context.template get_parameters<Parameters>();
  for(auto const& p: update) {
    if (p.label == "min_spread"sv) {
      parameters.min_spread = core::PriceUnits::parse(p.value);
    } else if (p.label == "max_spread"sv) {
      parameters.max_spread = core::PriceUnits::parse(p.value);
    }
  }
  return false;
}

} // namespace roq::pricer::ops