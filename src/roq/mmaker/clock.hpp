#pragma once
#include <chrono>

namespace roq::mmaker {

struct Clock {
    static std::chrono::nanoseconds now() {
        return std::chrono::system_clock::now().time_since_epoch();
    }
};

}