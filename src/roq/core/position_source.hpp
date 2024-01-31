// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include <magic_enum.hpp>

namespace roq {
namespace core {

enum class PositionSource {
    UNDEFINED,
    ORDERS,
    TRADES,
    ACCOUNT
};

enum class PositionSnapshot {
    UNDEFINED,
    PORTFOLIO,
    ACCOUNT
};

} // core
} // roq

ROQ_CORE_FMT_DECL(roq::core::PositionSource, "{}", magic_enum::enum_name(_));