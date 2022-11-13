#pragma once
namespace roq {
namespace mmaker {

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

} // mmaker
} // roq