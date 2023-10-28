#include "roq/pricer/ops/target_spread.hpp"
#include "roq/pricer/ops/mid_price.hpp"

namespace roq::pricer::ops {

bool TargetSpread::operator()(pricer::Context &context, pricer::Node const& node, pricer::Manager& manager) const {
  State const& state = node.fetch_parameter<State>(context.key);

  core::BestQuotes &q = context.quotes;
  core::Price mid_price = ops::MidPrice(q);

  if (core::is_empty_value(mid_price)) {
    context.quotes.clear();
  }

  switch (state.price_units) {
    case pricer::PriceUnits::BP: {
        q.bid.price = mid_price / (1. + .5 * state.target_spread * BP);
        q.ask.price = mid_price * (1. + .5 * state.target_spread * BP);
    } break;
    case pricer::PriceUnits::PRICE: {
        q.bid.price = mid_price - 0.5 * state.target_spread;
        q.ask.price = mid_price + 0.5 * state.target_spread;
    } break;
    default: {
        throw roq::RuntimeError("UNEXPECTED");
    }
  }
  return true;
}

bool TargetSpread::operator()(pricer::Context &context, roq::ParametersUpdate const& )  {
    return false;
}

} // namespace roq::pricer::ops