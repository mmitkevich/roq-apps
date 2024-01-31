#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <cstdint>
#include <string_view>
namespace roq::dag {

#ifndef ROQ_GRAPH_DEBUG
#define ROQ_GRAPH_DEBUG(fmt, ...) std::cout << std::format(fmt, __VA_ARGS__) << std::endl;
#endif

template<class NodeIdent = uint32_t>
struct Graph {
    using node_id_t = NodeIdent;

    Graph() = default;

    Graph(std::multimap<NodeIdent, NodeIdent>  && edges) 
    : edges(std::move(edges) ){}

    void reverse() {
        std::multimap<NodeIdent, NodeIdent> reversed;
        for(auto const & [src, dst] : edges) {
            reversed.emplace(dst, src);
        }
        std::swap(edges, reversed);
    }


    Graph(std::multimap<NodeIdent, NodeIdent> const & deps) {
        for(auto const & [node, dep_node] : deps) {
            edges.emplace(dep_node, node);
        }
    }
    
    enum ErrorCode {
        OK = 0,
        CYCLE = 1
    };

    ErrorCode build_path(node_id_t start, std::vector<node_id_t> & result) {
        using namespace std::literals;
        std::unordered_map<node_id_t, std::size_t> max_distance;
        std::unordered_set<node_id_t> todo;
        std::size_t num_edges = edges.size();
        todo.insert(start);
        max_distance[start] = 0;
        std::vector<std::unordered_set<node_id_t>> path;
        std::size_t mdist = 0;
        while(!todo.empty()) {
            node_id_t s = *todo.begin();
            todo.erase(todo.begin());
            auto r = edges.equal_range(s);
            for(auto it = r.first; it!=r.second; it++) {
                auto const & d = it->second;
                max_distance[d] = std::max(max_distance[s]+1, max_distance[d]);
                mdist = std::max(mdist, max_distance[d]);
                ROQ_GRAPH_DEBUG("build_path({}): max_dist({})={}"sv, start, d, max_distance[d]);
                if(mdist>num_edges) {
                    ROQ_GRAPH_DEBUG("build_path({}): cycle detected mdist {} num_edges {}"sv, start, mdist, num_edges);
                    return ErrorCode::CYCLE;
                }
                todo.insert(d);            
            }
        }
        for(auto& [n, dist] : max_distance) {
            while(path.size()<dist+1)
                path.push_back({});
            path[dist].insert(n);
        }
        result.clear();
        for(auto const & p: path) {
            for(auto const& n: p) {
                result.push_back(n);
            }
        }
        return ErrorCode::OK;
    }

    std::multimap<node_id_t, node_id_t> edges;
};

} // roq::dag