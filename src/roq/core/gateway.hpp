#pragma once
#include <roq/client.hpp>
#include <roq/logging.hpp>

#include <roq/cache/gateway.hpp>
#include <absl/container/flat_hash_map.h>
#include <roq/string_types.hpp>

#include <roq/cache/gateway.hpp>

namespace roq::core {

struct Gateway {
    cache::Gateway gateway;
    operator cache::Gateway&() { return gateway; }
    operator const cache::Gateway&() const { return gateway; }
    roq::Source gateway_name {};
    uint32_t gateway_id = -1;
};

}