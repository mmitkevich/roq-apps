// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include <roq/execution_instruction.hpp>
#include <roq/mask.hpp>

#include "roq/core/types.hpp"
#include "roq/core/empty_value.hpp"

namespace roq::core {

struct Quote {
    Price price {};
    Volume volume {};     

    bool empty() const { return core::is_empty_value(price); }
    
    void clear() {
        price = {};
        volume = {};
    }

    bool operator==(const Quote& that) const {
        return price == that.price && volume == that.volume;
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

struct ExecQuote  {
    core::Price price {};
    core::Volume volume {};
    core::Volume volume_min {};
    roq::Mask<roq::ExecutionInstruction> exec_inst;

    bool empty() const { return core::is_empty_value(price); }
    
    void clear() {
        price = {};
        volume = {};
        volume_min = {};
        exec_inst = {};
    }

    operator core::Quote() const {
        return core::Quote {
            .price = price,
            .volume = volume
        };
    }

    bool operator==(const Quote& that) const {
        return price == that.price && volume == that.volume;
    }

    bool operator!=(const Quote& that) const {
        return !operator==(that);
    }
};


} // roq::core

ROQ_CORE_FMT_DECL(roq::core::Quote, "{{ price {} volume {} }}", _.price, _.volume)
ROQ_CORE_FMT_DECL(roq::core::ExecQuote, "{{ price {} volume {} min {} exec_inst {} }}", _.price, _.volume, _.volume_min, _.exec_inst)