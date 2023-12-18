#include "roq/core/best_quotes.hpp"
#include "roq/dag/compute.hpp"
#include "roq/dag/ops/quote_shift.hpp"
#include "roq/core/price.hpp"
#include "roq/dag/node.hpp"
#include "roq/dag/pricer.hpp"
#include "roq/logging.hpp"

namespace roq::dag::ops {

bool QuoteShift::operator()(dag::Context &context) const {
  auto& params = context.get_parameters<Parameters>();

  core::BestQuotes& q = context.quotes;

  core::Shift shift = 0;

    // use exposure refs to shift prices
  context.get_refs([&](dag::NodeRef const& ref, dag::Node const& ref_node) {  
    const core::BestQuotes& ref_quotes = ref_node.quotes;
    if(ref_node.flags.has(NodeFlags::EXPOSURE)) {
        auto exposure = core::ExposureFromQuotes(ref_quotes);
        shift += ref.weight / ref.denominator * exposure;
        log::debug("quote_shift: shift {}  = weight {} / denominator {} * exposure {}", shift, ref.weight, ref.denominator, exposure);
    }
  });
  
  q.buy.price = core::shift_price(q.buy.price, shift );
  q.sell.price = core::shift_price(q.buy.price, shift );

  return true;
}

bool QuoteShift::operator()(dag::Context &context, std::span<const roq::Parameter>  update)  {
  using namespace std::literals;
  auto& parameters = context.template get_parameters<Parameters>();
  return false;
}

} // namespace roq::dag::ops