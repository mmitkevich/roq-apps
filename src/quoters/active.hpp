#pragma once
#include <utility>

#include "mixer/mixer-v1.hpp"

namespace umm {
namespace quoters {
    
template<class...Args>
auto mixer(Args...args) { return umm::quoters::mixer_v1(std::forward<Args>(args)...); }

} // quoters
} // qumm