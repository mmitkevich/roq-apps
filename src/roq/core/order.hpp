#pragma once

#include "roq/core/types.hpp"
#include "roq/side.hpp"
#include "roq/mask.hpp"
#include "roq/execution_instruction.hpp"

namespace roq::core {
 
struct Order {
    //core::OrderIdent order_id {};
    core::MarketIdent market {};
    roq::Side side {};
    core::Double quantity {};
    core::Double price {};
    std::string_view exchange;
    std::string_view symbol;
    std::string_view account;
    core::PortfolioIdent portfolio;
    roq::Mask<roq::ExecutionInstruction> exec_inst = {};
};

using TargetOrder = Order;

} // roq::core