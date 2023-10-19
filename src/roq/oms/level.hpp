#pragma once

#include "roq/mask.hpp"
#include "roq/execution_instruction.hpp"

namespace roq::oms {

struct Level {
    double price = NaN;
    double quantity = 0;
    double target_quantity = 0;
    double expected_quantity = 0;
    double confirmed_quantity = 0;
    roq::Mask<roq::ExecutionInstruction> exec_inst {};
};

}