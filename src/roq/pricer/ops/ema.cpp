#include "roq/pricer/ops/ema.hpp"
#include "roq/pricer/manager.hpp"


namespace roq::pricer::ops {

bool EMA::operator()(pricer::Context& context, pricer::Node const& node, pricer::Manager & manager) const {
    core::BestQuotes& q = context.quotes;
    core::Double const& omega = node.fetch_parameter<core::Double>(context.key);
    
    core::Quote const& bid = node.quotes.bid;
    core::Quote const& ask = node.quotes.ask;

    q.bid.price =  q.bid.price * omega + bid.price * (1.-omega);
    q.ask.price =  q.ask.price * omega + ask.price * (1.-omega);
    return true;
}

} // roq::pricer::ops