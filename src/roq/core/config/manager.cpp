#include <roq/update_type.hpp>
#include <string_view>

#include "roq/core/config/manager.hpp"
#include <roq/message_info.hpp>
#include <roq/parameter.hpp>
#include <roq/parameters_update.hpp>
#include "roq/core/string_utils.hpp"

namespace roq::core::config {

using namespace std::literals;

bool Query::operator()(roq::Parameter const& item) const {
    if(!label.empty() && item.label!=label)
        return false;
    if(!account.empty() && item.account!=account)
        return false;
    if(!symbol.empty() && item.symbol!=symbol)
        return false;
    if(!exchange.empty() && item.exchange!=exchange)
        return false;
    if(strategy_id && item.strategy_id!=strategy_id)
        return false;
    return true;
}

void Manager::load(std::string_view url) {
    this->url = std::string {url};
    toml = TomlFile { this->url };

    toml.get_nodes("parameter", [&](auto node) {
        roq::Parameter p;
        p.label = toml.get_string(node, "label"sv);
        p.exchange = toml.get_string_or(node, "exchange"sv, "");
        p.strategy_id = core::parse_uint32(toml.get_string_or(node, "strategy"sv, "0"));
        auto add_all_symbols = [&]() {
            toml.get_pairs(type_c<std::string>{}, node, "symbol"sv, "value"sv, [&](auto symbol, auto value) {
                p.symbol = symbol;
                p.value = value;
                this->parameters.push_back(p);
                log::debug("config load {}", p);
            });
        };

        if(0==toml.get_values(type_c<std::string>{}, node, "account"sv, [&](auto i, auto account) {
            p.account = account;
            add_all_symbols();    
        })) {
            p.account = {};
            add_all_symbols();
        }
        //p.symbol = toml.get_string(node, "symbol"sv);
        //p.value = toml.get_string(node, "value"sv);
    });
}

void Manager::dispatch(config::Handler& handler) {
    is_downloading = true;
    roq::ParametersUpdate update {
        .parameters = parameters,
        .update_type = UpdateType::SNAPSHOT,
        .user = user
    };
    roq::MessageInfo info;
    roq::Event event {info, update};
    handler(event);
    parameters.clear();
    is_downloading = false;
}


void Manager::operator()(roq::Event<roq::ParametersUpdate> const & event) {
    if(is_downloading)
        return;
    for(auto & u: event.value.parameters) {
        bool found = false;
        Query query {
            .label = u.label,            
            .symbol = u.symbol,
            .exchange = u.exchange,
            .account = u.account,            
            .strategy_id = u.strategy_id
        };
        for(auto & p: parameters) {
            if(query(p)) {
                found = true;
                p = u;
            }
        }
        if(!found) {
            parameters.push_back(u);
        }
    }
}


void Manager::dispatch(client::Config::Handler& handler) const {
    handler(client::Settings {
        .order_cancel_policy = OrderCancelPolicy::BY_ACCOUNT
    });

    toml.get_nodes("market", [&](auto node) {
        std::string exchange = toml.get_string(node, "exchange"sv);
        toml.get_values(type_c<std::string>{}, node, "symbol"sv, [&](auto i, auto symbol) {
            log::info<1>("config::Manager::dispatch symbol={}, exchange={}"sv, symbol, exchange);
            handler(client::Symbol {
                .regex = symbol,
                .exchange = exchange
            });
        });
    });
    toml.get_nodes("account", [&](auto node) {
        std::string account = toml.get_string(node, "account"sv);
        std::string exchange = toml.get_string(node, "exchange"sv);
        log::info<1>("config::Manager::dispatch account {} exchange {}"sv, account, exchange);
        handler(client::Account {
            .regex = account
        });
    });
}

} // roq::config