#pragma once
#include "roq/pricer/node.hpp"
#include <roq/parameters_update.hpp>

namespace roq::pricer::ops {

struct Product final : pricer::Compute  {
    static constexpr std::string_view NAME = "product";
    bool operator()(pricer::Context& context, pricer::Node const& node, pricer::Manager & manager) const override;
};

} // roq::pricer::compute