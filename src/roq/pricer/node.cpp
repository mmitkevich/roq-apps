#include "roq/core/best_quotes.hpp"
#include "roq/pricer/node.hpp"
#include "roq/pricer/manager.hpp"
#include <roq/exceptions.hpp>

namespace roq::pricer {


bool empty_compute(pricer::Context& context, pricer::Node const& node, pricer::Manager & manager) { return false; }

bool Node::operator()(roq::Event<roq::ParametersUpdate> const& e) { 
    pricer::Context context {};
    bool invalid = false;
    get_pipeline([&](Compute& comp) {
        if(!comp(context, e.value)) {
            invalid = true;
            context.clear();
        }
    });
    if(invalid) {
        return this->update(context);
    }
    return false;
}

bool Node::compute(pricer::Manager& manager) {
    pricer::Context context {};
    get_pipeline([&](Compute& comp) {
        if(!comp(context, *this, manager)) {
            context.clear();
        }
    });
    return this->update(context);
}

bool Node::update(Context const &rhs) {
    quotes = rhs.quotes;
    exposure = rhs.exposure;
    return true;
}

bool Node::update(core::Quotes const &rhs) { 
    return quotes.update(rhs); 
}

void Node::clear() {
    quotes = {};
    exposure = {};
}
void Context::clear() {
    quotes = {};
    exposure = {};
}

} // namespace roq::pricer