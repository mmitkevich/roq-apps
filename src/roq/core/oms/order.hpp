// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include <roq/side.hpp>
#include <roq/error.hpp>
#include <roq/string_types.hpp>
#include "roq/core/types.hpp"
#include "roq/core/oms/order_version.hpp"

namespace roq::core::oms {

using OrderIdent = core::OrderIdent;

struct Order {
    OrderIdent order_id {0};
    Side side = Side::UNDEFINED;

    double traded_quantity = NaN;
    double remaining_quantity = NaN;

    OrderVersion    pending {};     // pending order state (create/cancel/modify) and associated request type
    OrderVersion    confirmed {};   // cofirmed (via OrderUpdate) order state and request type succeeded
    OrderVersion    expected {};    // expected order state, which could be either pending or confirmed, including request type
    uint32_t    accept_version {0};
    uint32_t    reject_version {0};
    roq::Error  reject_error {roq::Error::UNDEFINED};
    std::string reject_reason {};

    uint32_t  strategy_id {0};
    roq::ExternalOrderId external_order_id;
    uint32_t rejects_count {0};

    bool is_pending() const { return pending.type!=RequestType::UNDEFINED ; }
    bool is_confirmed() const { return confirmed.type!=RequestType::UNDEFINED; }
};

}