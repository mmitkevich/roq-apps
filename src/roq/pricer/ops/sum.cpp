#include "roq/pricer/ops/sum.hpp"
#include "roq/pricer/manager.hpp"

namespace roq::pricer::ops {

bool Sum::operator()(pricer::Context& context) const {
    core::BestQuotes& q = context.quotes;

    q.buy = { .price = 0.0, .volume = 1.0 };
    q.sell = { .price = 0.0, .volume = 1.0 };

    context.manager.get_refs(context.node, [&](pricer::NodeRef const& ref, pricer::Node const& ref_node) {
        const core::Double w = ref.weight;
        const auto& buy = ref_node.quotes.buy;
        const auto& sell = ref_node.quotes.sell;

        if(w > 0) {
            q.buy.price += buy.price * w;
            q.sell.price += sell.price * w;
        } else if(w < 0) {
            q.buy.price += sell.price * w;
            q.sell.price += buy.price * w;
        }
    });
    return true;
}

} // roq::pricer::ops