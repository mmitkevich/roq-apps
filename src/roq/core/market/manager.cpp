// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/market/manager.hpp" 
#include <roq/exceptions.hpp>

namespace roq::core::market {

std::pair<core::market::Info&, bool> market::Manager::emplace_market(core::Market const&market) {
    auto market_id  = market.market;
    if(0==market_id) {
      // find by symbol/exchange index
      market_id = get_market_ident(market.symbol, market.exchange);
      if(0==market_id) {
        // not found in index => new market, insert
          market_id = ++last_market_id;
          auto &market_2 = market_by_id_[market_id];
          market_2.symbol = market.symbol;
          market_2.exchange = market.exchange;
          market_2.market = market_id; 
          market_by_symbol_by_exchange_[market_2.exchange][market_2.symbol] = market_id;     
          log::debug("emplace_market: market_id {} exchange {} symbol {}", market_id, market_2.exchange, market_2.symbol);
          return {market_2, true};
      }
    } 
    // market_id !=0 => should exist
    auto iter = market_by_id_.find(market_id);
    if(iter==std::end(market_by_id_))
      throw roq::RuntimeError("UNEXPECTED");
    auto& market_2 = iter->second;
    assert(market_2.market == market_id);
    return {market_2, false};
}

void market::Manager::operator()(const Event<GatewaySettings> &event) {
  for (auto &[market_id, market] : market_by_id_) {
    if (event.message_info.source_name == market.mdata_gateway_name) {
      market.mdata_gateway_id = event.message_info.source;
      log::info<1>("market::Manager: assigned mdata_gateway_id {} mdata_gateway {}",
                   market.mdata_gateway_id, market.mdata_gateway_name);
    }
    //if (event.message_info.source_name == market.trade_gateway_name) {
    //  market.trade_gateway_id = event.message_info.source;
    //  log::info<1>("market::Manager: assigned trade_gateway_id {} trade_gateway {}",
    //               market.trade_gateway_id, market.trade_gateway_name);
    //}
  }
}

void market::Manager::operator()(const Event<ReferenceData> &event) {
  auto [market, is_new] = emplace_market(event);
  market.tick_size = event.value.tick_size;
}


void market::Manager::clear() {
  market_by_symbol_by_exchange_.clear();
  market_by_id_.clear();
}

core::MarketIdent market::Manager::get_market_ident(std::string_view symbol, std::string_view exchange) const {
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


void Manager::configure(const config::TomlFile& config, config::TomlNode root) {
    clear();
    config.get_nodes(root, "market",[&](auto node) {
        using namespace std::literals;
        auto exchange = config.get_string(node, "exchange");
        auto mdata_gateway_name = config.get_string_or(node, "mdata_gateway", "");
        config.get_values(type_c<std::string>{}, node, "symbol"sv, [&](auto i, auto symbol) {
            core::MarketIdent market = this->get_market_ident(symbol, exchange);
            bool is_regex = symbol.find('*')!=std::string_view::npos;
            log::info<1>("symbol {}, exchange {}, market {} mdata_gateway '{}' is_regex {}", 
                symbol, exchange, market, 
                mdata_gateway_name, is_regex);
            if(is_regex)
              return;
            auto [item, is_new] = emplace_market({.market=market, .symbol=symbol, .exchange=exchange});
            item.pub_price_source = config.get_value_or(node, "pub_price_source", core::BestQuotesSource::UNDEFINED);
            item.best_quotes_source = config.get_value_or(node, "best_quotes_source", core::BestQuotesSource::TOP_OF_BOOK);
            item.lot_size = config.get_value_or(node, "lot_size", core::Volume{1.0});
            item.mdata_gateway_name = mdata_gateway_name;
            item.depth_num_levels = config.get_value_or(node, "depth_num_levels", core::Integer{0});
        });
    });    
}


} // roq::core