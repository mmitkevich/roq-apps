
#include "roq/core/types.hpp"
#include "roq/quoter/quoter.hpp"

namespace roq::quoter {

Quoter::Quoter(Dispatcher &dispatcher) 
: dispatcher(dispatcher) 
{}

void Quoter::operator()(const roq::Timer &timer) {

}

void Quoter::operator()(const core::Exposure &exposure) {
    auto [node, is_new] = emplace_node(exposure.market);
    node.exposure = exposure.exposure;
}

void Quoter::operator()(const core::Quotes &quotes) {
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
            dispatcher.send(core::Quotes {
                .bids = std::span {&node.bid, 1}, 
                .asks = std::span {&node.ask, 1}
            });
        }
    });
}

Node *Quoter::get_node(core::MarketIdent market) {
  auto iter = nodes.find(market);
  if (iter == std::end(nodes))
    return &iter->second;
  return nullptr;
}

std::pair<Node&, bool> Quoter::emplace_node(core::MarketIdent market, Node&& node) {
  auto [iter, is_new] = nodes.try_emplace(market, std::move(node));
  return {iter->second, is_new};
}

} // namespace roq::quoter