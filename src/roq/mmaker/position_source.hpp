#pragma once
namespace roq {
namespace mmaker {

enum class PositionSource {
    UNDEFINED,
    ORDERS,
    TRADES,
    POSITION
};

enum class PositionSnapshot {
    UNDEFINED,
    PORTFOLIO,
    ACCOUNT
};

} // mmaker
} // roq