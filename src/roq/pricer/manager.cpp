#include "roq/core/types.hpp"
#include "roq/pricer/manager.hpp"
#include "roq/pricer/aggr/sum.hpp"
#include "roq/pricer/aggr/product.hpp"
#include <roq/message_info.hpp>
//#include "roq/core/dispatcher.hpp"

namespace roq::pricer {

Manager::Manager(pricer::Handler& handler, core::Manager& core ) 
: handler(handler) 
, core(core)
{}

void Manager::operator()(const Event<roq::Timer> &event) {

}

void Manager::operator()(const Event<core::ExposureUpdate> &event) {
    for(auto& exposure : event.value.exposure) {
        auto [node, is_new] = emplace_node(exposure.market);
        node.exposure = exposure.exposure;
    }
}

void Manager::operator()(const roq::Event<roq::MarketStatus>&) {

}

void Manager::operator()(const Event<core::Quotes> &event) {
    auto & quotes = event.value;
    auto [node, is_new] = emplace_node(quotes.market);
    node.bid = quotes.bids[0];
    node.ask = quotes.asks[0];
    /// compute dependents and publish
    get_path(quotes.market, [&](core::MarketIdent item) {
        bool changed = false;
        Context context{node, *this};
        for(auto& compute: node.pipeline) {
            if(compute(context))
                changed = true;
        }
        if(node.market && changed) {
            target_quotes(node);
        }
    });
}

void Manager::target_quotes(Node& node) {
    core::TargetQuotes quotes {
        .bids = std::span {&node.bid, 1}, 
        .asks = std::span {&node.ask, 1}
    };
    roq::MessageInfo info;
    roq::Event event {info, quotes};
    handler(event);
}

Node *Manager::get_node(core::MarketIdent market) {
  auto iter = nodes.find(market);
  if (iter == std::end(nodes))
    return &iter->second;
  return nullptr;
}

std::pair<Node&, bool> Manager::emplace_node(core::MarketIdent market, Node&& node) {
  auto [iter, is_new] = nodes.try_emplace(market, std::move(node));
  return {iter->second, is_new};
}

} // namespace roq::pricer
