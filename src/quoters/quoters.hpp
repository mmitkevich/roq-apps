#pragma once
#include "factory.hpp"

#include "unstable/crypto_mixer.hpp"

namespace umm {
namespace quoters {

static inline Factory<
  CryptoMixerUnstable
> factory;

} // quoters
} // umm
