#include <string_view>

#include "roq/config/manager.hpp"
#include <roq/message_info.hpp>
#include <roq/parameter.hpp>
#include <roq/parameters_update.hpp>

namespace roq::config {

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
    return true;
}

void Manager::load(std::string_view url) {
    this->url = std::string {url};
    toml = TomlFile { this->url };
}

void Manager::dispatch(Handler& handler) {
    is_downloading = true;
    roq::ParametersUpdate update {
        .parameters = parameters,
        .update_type = UpdateType::SNAPSHOT,
        .user = user
    };
    roq::MessageInfo info;
    roq::Event event {info, update};
    handler(event);
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
        std::string symbol = toml.get_string(node, "symbol"sv);
        
        log::info<1>("config::Manager::dispatch symbol={}, exchange={}"sv, symbol, exchange);
        handler(client::Symbol {
            .regex = symbol,
            .exchange = exchange
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