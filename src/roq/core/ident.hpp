#pragma once
#pragma once

#ifdef USE_UMM
#include <umm/core/type.hpp>
#endif

#include <cstdint>

namespace roq {
namespace core {
#ifdef USE_UMM
    using MarketIdent = umm::MarketIdent;
#else
    using MarketIdent = uint32_t;
#endif

} // core
} // roq