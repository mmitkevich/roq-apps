// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/types.hpp"
#include <roq/logging.hpp>

#define ROQ_GRAPH_DEBUG(fmt,...)
// log::debug(fmt, #__VA_ARGS__)

#include "roq/dag/graph.hpp"

#include "roq/dag/pricer.hpp"
#include <roq/exceptions.hpp>
#include <roq/message_info.hpp>
#include <roq/parameters_update.hpp>
#include <roq/string_types.hpp>
#include "roq/dag/factory.hpp"
#include "roq/dag/node.hpp"
//#include "roq/core/dispatcher.hpp"
#include "roq/core/string_utils.hpp"
#include "roq/mask.hpp"

namespace roq::dag {


void Pricer::operator()(const Event<roq::Timer> &event) {

}

void Pricer::operator()(const Event<core::ExposureUpdate> &event) {
    for(auto& e : event.value.exposure) {
        // TODO: find all nodes subscribed to e.portfolio and update their exposure
    }
}

void Pricer::operator()(const roq::Event<roq::MarketStatus>&) {

}

void Pricer::operator()(const roq::Event<roq::ParametersUpdate>& e) {
    using namespace std::literals;    
    
    log::debug("pricer: parameters_update {}"sv, e);


    for(const roq::Parameter& p: e.value.parameters) {
        std::string node_name = fmt::format("{}:{}", p.exchange, p.symbol);
        auto [node, is_new] = emplace_node({
            .name = node_name
        });
        log::debug("pricer param label {} exchange {} symbol {} value {}", p.label, p.exchange, p.symbol, p.value);
        // label = ref.weight mdata exec compute.parameter
        //for(const auto tok : std::views::split(std::string_view{p.label}, '.')) {
        //    auto token = std::string_view { tok.data(), tok.size() };
        auto[label, suffix] = core::split_suffix(p.label, ':');
        if(label == "mdata"sv) {
            auto [exchange, symbol] = core::split_prefix(p.value, ':');
            set_mdata(node.node, {
                .symbol = symbol,
                .exchange = exchange
            });
        } else if(label == "portfolio_name"sv) {
            set_portfolio(node.node, {.portfolio_name = p.value});
        } else if(label == "ref"sv) {
            set_ref(node.node, p.value, suffix);
            // TOOD: add refs
        } else if(label == "pipeline"sv) {
            auto vec = core::split_sep(p.value, ' ');
            node.set_pipeline(vec);
        } else {
            node.update(std::span {&p, 1});
        }   
        //}
        fail:;
    }
}


void Pricer::operator()(const Event<core::Quotes> &event) {
    auto & quotes = event.value;
    log::debug<2>("dag::Quotes quotes={}", quotes);
    
    // forward quotes to input node (FIXME: only single input node per market)
    get_node_by_market(quotes.market, [&](Node& input_node) {
        input_node.update(quotes);
        // compute dependents and publish
        get_path(input_node.node, [&](core::NodeIdent node_id) {
            bool changed = false;
            Node* node = get_node(node_id);
            assert(node);
            Context context {
                .manager = *this,
                .node = *node
            };
            for(auto& compute: (*node).pipeline) {
                if((*compute)(context))
                    changed = true;
            }
            if((*node).exec && changed) {
                send_target_quotes((*node).node);
            }
        });
//    send_target_quotes(node);
    });
}

void Pricer::send_target_quotes(core::NodeIdent node_id) {
    auto* node = get_node(node_id);
    assert(node);
    assert((*node).exec!=0);
    log::debug<2>("dag::TargetQuotes quotes={}", (*node).quotes);
    core.markets.get_market((*node).exec, [&](auto& market) {
        core::TargetQuotes quotes {
            .market = market.market,
            .symbol = market.symbol,
            .exchange = market.exchange,
            .buy = std::span {&(*node).quotes.buy, 1}, 
            .sell = std::span {&(*node).quotes.sell, 1}
        };
        roq::MessageInfo info {};
        roq::Event event {info, quotes};
        dispatcher(event);
    });
}

Node *Pricer::get_node(core::NodeIdent node) {
  auto iter = nodes.find(node);
  if (iter != std::end(nodes))
    return &iter->second;
  return nullptr;
}

std::pair<dag::Node&, bool> Pricer::emplace_node(dag::NodeKey key) {
    auto node_id = key.node;
    if(node_id==0) {
        auto iter = node_by_name.find(key.name);
        if(iter!=std::end(node_by_name)) {
            node_id = iter->second;
        }
    }
    if(node_id==0) {
        node_id = ++last_node_id;
    }
    auto [iter, is_new_node] = nodes.try_emplace(node_id, *this);
    auto& node = iter->second;    
    if(is_new_node) {
        node.node = node_id;
        node.name = key.name;
        log::debug("pricer: emplace node.{} {}", node.node, node.name);
    }
    return {node, is_new_node};
}

bool Pricer::set_mdata(core::NodeIdent node_id, core::Market const& market_key) {
    auto* node = get_node(node_id);
    assert(node);

    if((*node).flags.has(NodeFlags::INPUT)) {
        throw roq::RuntimeError("pricer: input for node already specified");
    }

    auto [market, is_new_market] = core.markets.emplace_market(market_key);
    (*node).mdata = market.market;
    node_by_market[(*node).mdata] = (*node).node;
    if((*node).portfolio) {
        node_by_portfolio[(*node).portfolio][(*node).mdata] = (*node).node;
    }
    (*node).flags |= NodeFlags::INPUT;
    (*node).flags |= NodeFlags::PRICE;
    return true;
}


bool Pricer::set_ref(core::NodeIdent node_id, std::string_view ref_node_name, std::string_view flags_sv) {
    auto [ref_node, is_new] = emplace_node({.name = ref_node_name});
    std::string flags_str = core::to_upper(flags_sv);
    //TODO: parse FLAG1|FLAG2|FLAG3 mask
    std::optional<dag::NodeFlags> flags_opt = magic_enum::enum_cast<dag::NodeFlags>(flags_str);
    if(flags_opt.has_value()) {
        ref_node.flags.set(flags_opt.value());
    } else if (!flags_sv.empty()){
        log::info("pricer: failed to parse node flags {}", flags_sv);
    }
    auto* node = get_node(node_id);
    assert(node);
    log::debug("pricer: add ref node.{} -> node.{} flags {}", (*node).node, ref_node.node, ref_node.flags);    
    (*node).add_ref(ref_node.node, ref_node.flags);
    return true;
}

bool Pricer::set_portfolio(core::NodeIdent node_id, core::PortfolioKey const& portfolio_key) {
    auto* node = get_node(node_id);
    assert(node);
    if((*node).flags.has(NodeFlags::INPUT)) {
        throw roq::RuntimeError("input for node already specified");
    }
    auto [portfolio, is_new_portfolio] = core.portfolios.emplace_portfolio(portfolio_key);
    (*node).portfolio = portfolio_key.portfolio;
    if((*node).mdata) {
        node_by_portfolio[(*node).portfolio][(*node).mdata] = (*node).node;
    }
    (*node).flags |= NodeFlags::INPUT;    
    (*node).flags |= NodeFlags::EXPOSURE;
    return true;
}

void Pricer::rebuild_paths() {
    // vertex =  nodes
    // edges = node.refs for node in nodes
    // input vertex = node_by_market
    using node_id_t = core::NodeIdent;
    std::multimap<node_id_t, node_id_t> edges;
    get_nodes([&](Node const& node) {
        node.get_refs([&](NodeRef const & ref) {
            edges.emplace(ref.node, node.node);
        });
    });
    dag::Graph<core::NodeIdent> graph { std::move(edges) };
    for(auto& [market, node]: node_by_market) {
        graph.build_path(node, paths[node]);
    }
    
}

} // namespace roq::dag
