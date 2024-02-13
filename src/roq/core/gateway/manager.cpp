// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/gateway/manager.hpp"
#include <roq/message_info.hpp>

namespace roq::core::gateway {

bool gateway::Manager::is_ready(roq::Mask<SupportType> mask, uint32_t source) const {
  auto gateway_iter = gateway_by_id_.find(source);
  if (gateway_iter == std::end(gateway_by_id_)) {
    return false;
  }
  const core::Gateway &gateway = gateway_iter->second;
  const cache::Gateway & gateway_1 = gateway;
  return gateway_1.ready(mask);
}

bool gateway::Manager::is_ready(roq::Mask<SupportType> mask, uint32_t source, std::string_view account) const {
  auto gateway_iter = gateway_by_id_.find(source);
  if (gateway_iter == std::end(gateway_by_id_)) {
    return false;
  }
  const core::Gateway& gateway = gateway_iter->second;
  const cache::Gateway& gateway_1 = gateway;
  bool ready = gateway_1.ready(mask, account);
  if (!ready) {
    auto &state_by_account = gateway_1.state_by_account;
    auto iter = state_by_account.find(account);
    if (iter != std::end(state_by_account)) {
      auto &state = iter->second;
      log::info<2>("gateway::Manager: source {} account {} not ready downloading {} "
                   "available {} unavailable {} expected {}",
                   source, account, state.downloading, state.status.available,
                   state.status.unavailable, mask);
    } else {
      log::info<2>("gateway::Manager: source {} account {} not ready expected {}",
                   source, account, mask);
    }
  }
  return ready;
}

bool Manager::is_downloading(uint32_t source, std::string_view account) const {
  auto gateway_iter = gateway_by_id_.find(source);
  if (gateway_iter == std::end(gateway_by_id_)) {
    return true;
  }
  const core::Gateway& gateway = gateway_iter->second;
  const cache::Gateway& gateway_1 = gateway;
  auto &state_by_account = gateway_1.state_by_account;
  auto iter = state_by_account.find(account);
  if (iter != std::end(state_by_account)) {
    return iter->second.downloading;
  }
  return true;
}

bool gateway::Manager::is_downloading(uint32_t id) const {
  auto iter = gateway_by_id_.find(id);
  if (iter == std::end(gateway_by_id_)) {
    return false;
  }
  const core::Gateway &gateway = iter->second;
  const cache::Gateway& gateway_1 = gateway;
  return gateway_1.state.downloading;
}

std::pair<core::Gateway &, bool> gateway::Manager::emplace_gateway(roq::MessageInfo const& info) {
    auto iter = gateway_by_id_.find(info.source);  
    if( iter!= std::end(gateway_by_id_))
        return {iter->second, false};
    core::Gateway& gateway = gateway_by_id_[info.source];
    gateway.gateway_id = info.source;
    gateway.gateway_name = info.source_name;
    log::info<2>("gateway::Manager::emplace_gateway gateway_id={} gateway_name={}", gateway.gateway_id, gateway.gateway_name);
    return {gateway, true};
}

} // namespace roq::core