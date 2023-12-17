#include "roq/core/best_quotes.hpp"
#include "roq/dag/node.hpp"
#include "roq/dag/manager.hpp"
#include <roq/exceptions.hpp>
#include <roq/logging.hpp>
#include "roq/dag/factory.hpp"

namespace roq::dag {


//bool empty_compute(dag::Context& context, dag::Node const& node, dag::Manager & manager) { return false; }

bool Node::update(std::span<const roq::Parameter> params) { 
    auto context = make_context();
    // pass through parameters into pipeline members
    bool modified = false;
    get_pipeline([&](Compute& compute) {
        if(compute(context, params)) {
            modified = true;
        }
    });
    if(modified) {
        return this->update(context);
    }
    return false;
}

bool Node::compute() {
    auto context = make_context();
    get_pipeline([&](Compute& compute) {
        if(!compute(context)) {
            context.clear();
        }
    });
    return this->update(context);
}

bool Node::update(dag::Context const &rhs) {
    quotes = rhs.quotes;
    //exposure = rhs.exposure;
    return true;
}

bool Node::update(core::Quotes const &rhs) { 
    return quotes.update(rhs); 
}

void Node::clear() {
    quotes = {};
    //exposure = {};
}

void Node::set_pipeline(const std::span<std::string_view>  vec) {
    pipeline_size = vec.size();
    uint32_t offset = 0;
    for(uint32_t i=0; i<vec.size(); i++) {
        Compute* c= dag::Factory::get(vec[i]);
        if(!c) {
            log::error("pricer: compute not found {}", vec[i]);
            throw roq::RuntimeError("pricer: compute not found {}", vec[i]);
        }
        pipeline[i] = c;
    }
}


} // namespace roq::dag