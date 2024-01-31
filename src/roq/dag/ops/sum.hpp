#pragma once
#include "roq/dag/node.hpp"

namespace roq::dag::ops {

struct Sum : Compute {
    static constexpr std::string_view NAME = "sum";
    bool operator()(dag::Context& context) const override;
};

} // roq::dag::ops