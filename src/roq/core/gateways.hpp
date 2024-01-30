// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include "roq/core/hash.hpp"
#include <roq/client.hpp>
#include <roq/logging.hpp>

#include <roq/cache/gateway.hpp>
#include <absl/container/flat_hash_map.h>
#include <roq/message_info.hpp>
#include <roq/string_types.hpp>

#include "roq/core/gateway.hpp"
#include "roq/cache/gateway.hpp"

namespace roq::core {

struct Gateways {
    bool is_ready(roq::Mask<SupportType> mask, uint32_t source) const;

    bool is_ready(roq::Mask<SupportType> mask, uint32_t source, std::string_view account) const;

    bool is_downloading(uint32_t id) const;

    template<class Fn>
    bool get_gateway(uint32_t id, Fn&& fn) const {
        return id!=-1 && hash_get_value(gateway_by_id_, id, std::forward<Fn>(fn));
    }

    template<class Fn>
    bool get_gateway(std::string_view name, Fn&& fn) const {
        return hash_get_value(gateway_id_by_name_, name, std::forward<Fn>(fn));
    }

    std::pair<core::Gateway &, bool> emplace_gateway(roq::MessageInfo const& info);

    template<class T>
    bool operator()(const Event<T>& event) {
        bool result = false;
        if constexpr(std::is_invocable_v<cache::Gateway, const Event<T>&>) {
            if(event.message_info.source_name.size()) { // ignore empty source name -- means locally-generated message without any gateway
                auto [gateway, is_new] = emplace_gateway(event.message_info);
                cache::Gateway& gateway_2 = gateway;
                result = gateway_2(event);
            }
        }
        return result;
    }

  private:
    core::Hash<uint32_t, core::Gateway> gateway_by_id_;
    core::Hash<roq::Source, uint32_t> gateway_id_by_name_;
};

} // roq::core