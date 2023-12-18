#include "roq/dag/ops/product.hpp"
#include "roq/dag/pricer.hpp"

namespace roq::dag::ops {


bool Product::operator()(dag::Context& context) const {
    core::BestQuotes& q = context.quotes;

    q.buy = { .price = 1.0, .volume = 1.0 };
    q.sell = { .price = 1.0, .volume = 1.0 };

    context.get_refs([&](dag::NodeRef const& ref, dag::Node const& ref_node) {
        const core::Double w = ref.weight;
        const auto& buy = ref_node.quotes.buy;
        const auto& sell = ref_node.quotes.sell;
        if(w == 1) {
            q.buy.price *= buy.price;
            q.sell.price *= sell.price;
        } else if(w == -1) {
            q.buy.price /= sell.price;
            q.sell.price /= buy.price;
        } else if(w > 0) {
            q.buy.price *= std::pow(buy.price, w);
            q.sell.price *= std::pow(sell.price, w);
        } else if(w < 0) {
            q.buy.price *= std::pow(sell.price, w);
            q.sell.price *= std::pow(buy.price, w);
        }
        log::debug("product: buy {} sell {} *= weight {} * {{ buy {} sell {} }}", q.buy.price, q.sell.price, w, buy.price, sell.price);
    });
    return true;
}


} // roq::dag::ops