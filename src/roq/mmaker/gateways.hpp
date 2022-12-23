#pragma once
#include <roq/client.hpp>
#include <roq/logging.hpp>

#include <roq/cache/gateway.hpp>
#include <absl/container/flat_hash_map.h>
#include <roq/string_types.hpp>

namespace roq {
namespace mmaker {


struct Context;

struct Gateways {
    struct Item {
        cache::Gateway base;
        operator cache::Gateway&() { return base; }
        operator const cache::Gateway&() const { return base; }
        roq::Source name {};
        uint32_t id = -1;
    };

    template<class T>
    bool operator()(const Event<T>& event) {
        bool result = false;
        uint32_t gateway_id = event.message_info.source;
        auto [gateway, is_new_gateway] = get_gateway_or_create(gateway_id);
        if(is_new_gateway) {
            gateway.id = gateway_id;
            gateway.name = event.message_info.source_name;
            gateway_id_by_name_[gateway.name] = gateway_id;
            log::info<2>("Gateways::NewGateway gateway_id={} gateway_name={}", gateway.id, gateway.name);
        }

        if constexpr(std::is_invocable_v<cache::Gateway, const Event<T>&>) {
            result = gateway.base(event);
        }
        return result;
    }

    bool is_ready(roq::Mask<SupportType> mask, uint32_t source) const {
        auto gateway_iter = gateway_by_id_.find(source);
        if (gateway_iter == std::end(gateway_by_id_)) {
            return false;
        }
        auto& gateway = gateway_iter->second;
        return gateway.base.ready(mask);
    }
    
    bool is_ready(roq::Mask<SupportType> mask, uint32_t source, std::string_view account) const {
        auto gateway_iter = gateway_by_id_.find(source);
        if (gateway_iter == std::end(gateway_by_id_)) {
            return false;
        }
        auto &gateway = gateway_iter->second;
        bool ready =  gateway.base.ready(mask, account);
        if(!ready) {
            auto& state_by_account = gateway.base.state_by_account;
            auto iter = state_by_account.find(account);
            if(iter!=std::end(state_by_account)) {
                auto& state = iter->second;
                log::info<2>("Gateways: source {} account {} not ready downloading {} available {} unavailable {} expected {}", source, account, state.downloading, state.status.available, state.status.unavailable, mask);
            } else {
                log::info<2>("Gateways: source {} account {} not ready expected {}", source, account, mask);
            }
            
        }
        return ready;
    }

    bool is_downloading(uint32_t id) const {
        auto gateway_iter = gateway_by_id_.find(id);
        if (gateway_iter == std::end(gateway_by_id_)) {
            return false;
        }
        auto &gateway = gateway_iter->second;
        return gateway.base.state.downloading;
    }

    template<class Fn>
    bool get_gateway(uint32_t id, Fn&& fn) const {
        if(id==(-1))
            return false;
        auto iter = gateway_by_id_.find(id);
        if(iter!=std::end(gateway_by_id_)) {
            fn(iter->second);
            return true;
        }
        return false;
    }

    template<class Fn>
    bool get_gateway(std::string_view name, Fn&& fn) const {
        auto iter = gateway_id_by_name_.find(name);
        if(iter==std::end(gateway_id_by_name_))
            return false;
        fn(iter->second);
        return true;
    }

private:
    std::pair<Gateways::Item&, bool> get_gateway_or_create(uint32_t id) {
        auto iter = gateway_by_id_.find(id);
        if(iter==std::end(gateway_by_id_)) {
            return std::pair<Gateways::Item&, bool>{gateway_by_id_[id], true};
        }
        return std::pair<Gateways::Item&, bool>{iter->second, false};
    }
    
private:
    absl::flat_hash_map<uint32_t, Gateways::Item> gateway_by_id_;
    absl::flat_hash_map<roq::Source, uint32_t> gateway_id_by_name_;
};

}
}