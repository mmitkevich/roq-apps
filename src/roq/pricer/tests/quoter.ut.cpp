#include "roq/pricer/pricer.hpp"
#include "roq/core/manager.hpp"
#include "roq/pricer/aggr/sum.hpp"
#include "roq/pricer/aggr/product.hpp"

int main() {
    using namespace roq;
    core::Dispatcher* dispatcher = nullptr;
    core::Manager* cache = nullptr;
    pricer::Manager pricer(*dispatcher, *cache);

    pricer.emplace_node(10, {
        .bid = {.price = 10},
        .ask = {.price = 20}
    });

    pricer.emplace_node(20, {
        .bid = {.price = 30},
        .ask = {.price = 40}
    });

    pricer.emplace_node(100, {
        .bid = {.price = 100, .volume = 10},
        .ask = {.price = 110, .volume = 11},
        .refs = {
            pricer::Node::Ref {.market = 10, .weight = 0.5, .ctx=nullptr},
            {.market = 20, .weight = 0.5, .ctx=nullptr},
        },
        .pipeline = { pricer::aggr::Sum{}, pricer::aggr::Product{} }
    });
}