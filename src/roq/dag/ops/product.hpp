#pragma once
#include "roq/dag/node.hpp"
#include <roq/parameters_update.hpp>

namespace roq::dag::ops {

struct Product final : dag::Compute  {
    static constexpr std::string_view NAME = "product";
    bool operator()(dag::Context& context) const override;
};

} // roq::dag::compute