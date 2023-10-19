#pragma once

namespace roq::pricer::aggr {

struct Sum {

    template<class Context>
    bool operator()(Context& context) {
        auto& node = context.node;
        auto& pricer = context.pricer;
        node.bid.price = 0;
        node.ask.price = 0;
        for(auto& ref: node.refs) {
            auto* rnode = pricer.get_node(ref.market);
            assert(rnode);
            if(ref.weight>=0.) {
                node.bid.price += ref.weight * rnode->bid.price;
                node.ask.price += ref.weight * rnode->ask.price;
            } else {
                node.bid.price += ref.weight * rnode->ask.price;
                node.ask.price += ref.weight * rnode->bid.price;
            }
        }
        return true;
    }  
};


} // roq::pricer::aggr