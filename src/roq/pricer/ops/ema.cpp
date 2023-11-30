#include "roq/pricer/ops/ema.hpp"
#include "roq/pricer/manager.hpp"


namespace roq::pricer::ops {

bool EMA::operator()(pricer::Context& context) const {
    core::BestQuotes& q = context.quotes;
    auto& params = context.get_parameters<Parameters>();
    
    core::Quote const& buy = context.node.quotes.buy;
    core::Quote const& sell = context.node.quotes.sell;

    q.buy.price =  q.buy.price * params.omega + buy.price * (1.-params.omega);
    q.sell.price =  q.sell.price * params.omega + sell.price * (1.-params.omega);
    return true;
}

bool EMA::operator()(pricer::Context &context, std::span<const roq::Parameter>  update)  {
  using namespace std::literals;
  auto& parameters = context.template get_parameters<Parameters>();
  for(auto const& p: update) {
    if(p.label == "ema:omega"sv) {
      parameters.omega = core::Double::parse(p.value);
    }
  }
  return false;
}

} // namespace roq::pricer::ops