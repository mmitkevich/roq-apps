#pragma once
#include "roq/pricer/node.hpp"

namespace roq::pricer::ops {

struct Sum : Compute {
    static constexpr std::string_view NAME = "sum";
    bool operator()(pricer::Context& context, pricer::Node const& node, pricer::Manager & manager) const override;
};

} // roq::pricer::ops