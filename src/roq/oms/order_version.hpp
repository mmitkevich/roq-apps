#pragma once
#include "roq/core/types.hpp"
#include <roq/request_type.hpp>
#include <roq/order_status.hpp>
#include <roq/core/clock.hpp>
#include <roq/numbers.hpp>

namespace roq::oms {

using OrderVersionIdent = core::OrderVersionIdent;

struct OrderVersion  {
    RequestType type {RequestType::UNDEFINED};   // CREATE, MODIFY, CANCEL
    OrderStatus status {OrderStatus::UNDEFINED};
    OrderVersionIdent version = 0;
    double price = NaN;
    double quantity = NaN;
    std::chrono::nanoseconds created_time {};
    //void clear() { *this = OrderVersion {}; }
};

inline std::chrono::nanoseconds get_latency(oms::OrderVersion const& self)  {  return core::Clock::now() - self.created_time; }

} // roq::oms