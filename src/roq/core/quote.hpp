#pragma once
#include <roq/execution_instruction.hpp>
#include <roq/mask.hpp>

#include "roq/core/types.hpp"
#include "roq/core/empty_value.hpp"

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

template<>
inline bool is_empty_value(const core::Quote& quote) {
    if(is_empty_value(quote.price))
        return true;
    if(is_empty_value(quote.volume))
        return true;
    if(quote.volume==0.)
        return true;
    return false;
}

} // roq::core

ROQ_CORE_FMT_DECL(roq::core::Quote, "{{ price {} volume {} exec_inst {} }}", _.price, _.volume, _.exec_inst)