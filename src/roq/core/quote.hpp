#pragma once
#include "roq/core/types.hpp"
#include <roq/execution_instruction.hpp>
#include <roq/mask.hpp>

namespace roq::core {

struct Quote {
    Price price {};
    Volume volume {};        
    roq::Mask<roq::ExecutionInstruction> exec_inst {}; // TAKER/MAKER

    bool empty() const { return core::is_empty_value(price); }
    
    void clear() {
        price = {};
        volume = {};
        exec_inst = {};
    }

    bool operator==(const Quote& that) const {
        return price == that.price && volume == that.volume && exec_inst == that.exec_inst;
    }

    bool operator!=(const Quote& that) const {
        return !operator==(that);
    }
};

} // roq::core

ROQ_CORE_FMT_DECL(roq::core::Quote, "{{ price {} volume {} exec_inst {} }}", _.price, _.volume, _.exec_inst)