#include <iostream>
#include <format>

#include "graph.hpp"


int main(int argc, char*argv[]) {
    using node_id_t = uint32_t;
    namespace grf = roq::pricer;

    std::multimap<node_id_t, node_id_t> args {
        {3, 1},  {3, 2}, 
        {4, 1},  {4, 3}, 
        {5, 2},  {5, 4}, 
        {6, 3},  {6, 1}, 
        {7, 2}
    };

    for(auto &[l, r] : args) {
        std::cout << "args: "<<l<<" -> "<<r << "\n";
    }

    std::cout << "\n";

    grf::Graph<uint32_t> graph { std::move(args) };
    graph.reverse();    // build deps from args

    for(auto &[l, r] : graph.edges) {
        std::cout << "deps: "<<l<<" -> "<<r << "\n";
    }
    std::vector<node_id_t> nodes = {1, 2, 3, 4, 5};
    for(auto& n: nodes) {
        std::vector<node_id_t> path;
        graph.build_path(n, path);
        std::cout<<n<<" path = ";
        for(auto n: nodes) {
            std::cout <<n<<" ";
        }
        std::cout << "\n";
    }

    return 0;
}