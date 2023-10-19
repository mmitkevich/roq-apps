#include "roq/core/markets.hpp" 

namespace roq::core {

core::Market& Markets::emplace_market(core::Market const& market) {
    auto market_id = market.market;
    if(market_id==0) {
      market_id = ++last_market_id;
    }
    auto &market_2 = market_by_id_[market_id] = market;
    market_2.market = market_id;
    market_by_symbol_by_exchange_[market.exchange][market.symbol] = market_id;
    return market_2;
}

void Markets::operator()(const Event<GatewayStatus> &event) {
  for (auto &[market_id, market] : market_by_id_) {
    if (event.message_info.source_name == market.mdata_gateway_name) {
      market.mdata_gateway_id = event.message_info.source;
      log::info<1>("Markets: assigned mdata_gateway_id {} mdata_gateway {}",
                   market.mdata_gateway_id, market.mdata_gateway_name);
    }
    if (event.message_info.source_name == market.trade_gateway_name) {
      market.trade_gateway_id = event.message_info.source;
      log::info<1>("Markets: assigned trade_gateway_id {} trade_gateway {}",
                   market.trade_gateway_id, market.trade_gateway_name);
    }
  }
}

void Markets::clear() {
  market_by_symbol_by_exchange_.clear();
  market_by_id_.clear();
}

core::MarketIdent Markets::get_market_ident(std::string_view symbol, std::string_view exchange) const {
  auto iter_1 = market_by_symbol_by_exchange_.find(exchange);
  if (iter_1 == std::end(market_by_symbol_by_exchange_)) {
    return {};
  }
  auto &market_by_symbol = iter_1->second;
  auto iter_2 = market_by_symbol.find(symbol);
  if (iter_2 == std::end(market_by_symbol)) {
    return {};
  }
  return iter_2->second;
}

} // roq::core