// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include <chrono>

namespace roq::core {

struct Clock {
    static std::chrono::nanoseconds now() {
        return std::chrono::system_clock::now().time_since_epoch();
    }
};

} // roq::core