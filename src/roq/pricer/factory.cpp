#include "factory.hpp"

#include "ops/all.hpp"
#include "roq/pricer/ops/ema.hpp"
#include "roq/pricer/ops/product.hpp"
#include "roq/pricer/ops/sum.hpp"
#include "roq/pricer/ops/target_spread.hpp"

namespace roq::pricer {

void Factory::initialize_all() {
    initialize<ops::Sum, ops::Product, ops::EMA, ops::TargetSpread>();
}

static Factory default_factory;

Factory *Factory::g_instance_ = &default_factory;

void Factory::emplace(std::string_view name, std::unique_ptr<pricer::Compute> value) {
    g_instance_->registry_[name] = std::move(value);
}

void Factory::erase(std::string_view name) {
    g_instance_->registry_.erase(name);
}

pricer::Compute* Factory::get(std::string_view name) {
    auto iter = g_instance_->registry_.find(name);
    if(iter!=std::end(g_instance_->registry_))
        return iter->second.get();
    return nullptr;
}

}