#pragma once
#include "roq/pricer/node.hpp"
#include <roq/parameters_update.hpp>

namespace roq::pricer::ops {

struct Product final : pricer::Compute  {
    static constexpr std::string_view NAME = "product";
    bool operator()(pricer::Context& context) const override;
};

} // roq::pricer::compute