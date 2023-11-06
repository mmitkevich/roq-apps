// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/types.hpp"
#include "roq/pricer/manager.hpp"
#include <roq/exceptions.hpp>
#include <roq/message_info.hpp>
#include <roq/parameters_update.hpp>
#include <ranges>
#include <roq/string_types.hpp>
#include "roq/pricer/factory.hpp"
//#include "roq/core/dispatcher.hpp"

namespace roq::pricer {

Manager::Manager(pricer::Handler& handler, core::Manager& core ) 
: handler(&handler) 
, core(core)
{}

void Manager::operator()(const Event<roq::Timer> &event) {

}

void Manager::operator()(const Event<core::ExposureUpdate> &event) {
    for(auto& e : event.value.exposure) {
        auto [node, is_new] = emplace_node({
            .market = e.market, 
            .symbol = e.symbol,
            .exchange = e.exchange
        });
        node.exposure = e.exposure;
    }
}

void Manager::operator()(const roq::Event<roq::MarketStatus>&) {

}

std::pair<std::string_view, std::string_view> split_prefix(std::string_view input, char sep) {
    using namespace std::literals;
    auto pos = input.find(sep);
    if(pos==std::string_view::npos) {
        return {""sv, input};
    }
    return {std::string_view{input.data(), pos}, std::string_view{input.data()+pos+1, input.size()-pos-1}};
}

std::vector<std::string_view> split_sep(std::string_view str, char sep) {
    std::vector<std::string_view> result;
    for(const auto tok : std::views::split(std::string_view{str}, sep)) {
        result.push_back(std::string_view { tok.data(), tok.size() } );
    }
    return result;
}

void Manager::set_pipeline(pricer::Node & node, const std::vector<std::string_view> & pipeline) {
    node.pipeline_size = pipeline.size();
    uint32_t offset = 0;
    for(uint32_t i=0; i<pipeline.size(); i++) {
        node.pipeline[i] = pricer::Factory::get(pipeline[i]);
        node.pipeline[i]->offset = offset;
        offset+=node.pipeline[i]->size;
    }
}

void Manager::operator()(const roq::Event<roq::ParametersUpdate>& e) {
    using namespace std::literals;    
    
    log::debug("pricer: parameters_update {}"sv, e);

    static constexpr std::string_view MDATA="mdata"sv, EXEC="exec"sv, REF_WEIGHT="ref.weight"sv, PIPELINE="pipeline";

    get_nodes([&](auto& node) {
        for(const roq::Parameter& p: e.value.parameters) {
            auto [node, is_new] = emplace_node({
                        .symbol = p.symbol,
                        .exchange = p.exchange
                    });
            // label = ref.weight mdata exec compute.parameter
            for(const auto tok : std::views::split(std::string_view{p.label}, '.')) {
                auto token = std::string_view { tok.data(), tok.size() };
                bool is_mdata = (token == MDATA);
                if(is_mdata || token==EXEC) {
                    auto [exchange, symbol] = split_prefix(p.value, ':');
                    auto [market, is_new] = core.markets.emplace_market({
                        .symbol = symbol,
                        .exchange = exchange
                    });
                    node.market = market;
                    if(is_mdata) {
                        node.flags |= Node::Flags::INPUT;
                    }
                } else if(token == REF_WEIGHT) {
                    // TOOD: add refs
                } else if(token == PIPELINE) {
                    set_pipeline(node, split_sep(token, ' '));
                } else {
                    // TODO: change parameters
                }   
            }
            fail:;
        }
        node(e);
    });
}

void Manager::operator()(const Event<core::Quotes> &event) {
    auto & quotes = event.value;
    auto [node, is_new] = emplace_node({.market=quotes.market, .symbol=quotes.symbol, .exchange=quotes.exchange});
    node.update(quotes);

    log::debug<2>("pricer::Quotes Node market={} symbol={} exchange={} bid={} ask={}",node.market, quotes.symbol, quotes.exchange, node.quotes.bid, node.quotes.ask);
    
//    target_quotes(node);
/*
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
*/        
}

void Manager::target_quotes(Node& node) {
    assert(node.market.market!=0);
    core::TargetQuotes quotes {
        .market = node.market.market,
        .symbol = node.market.symbol,
        .exchange = node.market.exchange,
        .bids = std::span {&node.quotes.bid, 1}, 
        .asks = std::span {&node.quotes.ask, 1}
    };
    roq::MessageInfo info {};
    roq::Event event {info, quotes};
    log::debug<2>("pricer::handler={}",(void*)handler);
    (*handler)(event);
}

Node *Manager::get_node(core::MarketIdent market) {
  auto iter = nodes.find(market);
  if (iter == std::end(nodes))
    return &iter->second;
  return nullptr;
}

std::pair<pricer::Node&, bool> Manager::emplace_node(core::Market args) {
    auto node_id = args.market;
    if(node_id==0) {
        auto iter_1 = node_by_symbol_by_exchange_.find(args.exchange);
        if(iter_1!=std::end(node_by_symbol_by_exchange_)) {
            auto iter_2 = iter_1->second.find(args.symbol);
            if(iter_2!=std::end(iter_1->second)) {
                node_id = iter_2->second;
            }
        }
    }
    if(node_id==0) {
        node_id = ++last_node_id;
    }
    auto [iter, is_new_node] = nodes.try_emplace(node_id);
    auto& node = iter->second;    
    if(is_new_node) {
        node.exchange = args.exchange;
        node.symbol = args.symbol;
        node.node_id = node_id;
        node_by_symbol_by_exchange_[node.exchange][node.symbol] = node.node_id;
    }
    return {node, is_new_node};
}

} // namespace roq::pricer
