#pragma once
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