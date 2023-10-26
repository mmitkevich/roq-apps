// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include <cmath>

namespace roq::pricer::aggr {

struct Product {
    template<class Context>
    bool operator()(Context& context) {
        auto& node = context.node;
        auto& pricer = context.pricer;
        node.bid.price = 1.;
        node.ask.price = 1.;
        for(auto& ref: node.refs) {
            auto* rnode = pricer.get_node(ref.market);
            assert(rnode);
            if(ref.weight>=0.) {
                node.bid.price *= std::pow(rnode->bid.price,ref.weight);
                node.ask.price += std::pow(rnode->ask.price,ref.weight);
            } else {
                node.bid.price *= std::pow(rnode->ask.price,ref.weight);
                node.ask.price += std::pow(rnode->bid.price,ref.weight);
            }
        }
        return true;
    }
};


} // roq::pricer::aggr