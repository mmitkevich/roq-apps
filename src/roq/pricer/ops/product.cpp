#include "roq/pricer/ops/product.hpp"
#include "roq/pricer/manager.hpp"

namespace roq::pricer::ops {


bool Product::operator()(pricer::Context& context, pricer::Node const& node, pricer::Manager & manager) const {
    core::BestQuotes& q = context.quotes;

    q.bid = { .price = 1.0, .volume = 1.0 };
    q.ask = { .price = 1.0, .volume = 1.0 };

    manager.get_refs(node, [&](pricer::NodeRef const& ref, pricer::Node const& ref_node) {
        const core::Double w = ref.weight;
        const auto& bid = ref_node.quotes.bid;
        const auto& ask = ref_node.quotes.ask;
        if(w == 1) {
            q.bid.price *= bid.price;
            q.ask.price *= ask.price;
        } else if(w == -1) {
            q.bid.price /= ask.price;
            q.ask.price /= bid.price;
        } else if(w > 0) {
            q.bid.price *= std::pow(bid.price, w);
            q.ask.price *= std::pow(ask.price, w);
        } else if(w < 0) {
            q.bid.price *= std::pow(ask.price, w);
            q.ask.price *= std::pow(bid.price, w);
        }
    });
    return true;
}


} // roq::pricer::ops