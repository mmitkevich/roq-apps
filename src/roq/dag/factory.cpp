#include "factory.hpp"

#include "ops/all.hpp"
#include "roq/dag/ops/ema.hpp"
#include "roq/dag/ops/product.hpp"
#include "roq/dag/ops/quote_shift.hpp"
#include "roq/dag/ops/sum.hpp"
#include "roq/dag/ops/quote_spread.hpp"

namespace roq::dag {

void Factory::initialize_all() {
    initialize<ops::Sum, ops::Product, ops::EMA, ops::QuoteSpread, ops::QuoteShift>();
}

static Factory default_factory;

Factory *Factory::g_instance_ = &default_factory;

void Factory::emplace(std::string_view name, std::unique_ptr<dag::Compute> value) {
    g_instance_->registry_[name] = std::move(value);
}

void Factory::erase(std::string_view name) {
    g_instance_->registry_.erase(name);
}

dag::Compute* Factory::get(std::string_view name) {
    auto iter = g_instance_->registry_.find(name);
    if(iter!=std::end(g_instance_->registry_))
        return iter->second.get();
    return nullptr;
}

}