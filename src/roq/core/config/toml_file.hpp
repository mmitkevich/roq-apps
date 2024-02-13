#pragma once

#include "roq/core/type_list.hpp"
#include "roq/core/types.hpp"
#include "roq/core/hash.hpp"

#include <roq/exceptions.hpp>
#include "roq/logging.hpp"


#include <fmt/format.h>
#include <functional>
#include <sstream>
#include <string_view>

//#include <toml++/impl/forward_declarations.h>
//#include <toml++/impl/parser.h>
#include <toml++/toml.h>
//#include <type_traits>
//#include <magic_enum.hpp>

namespace roq::core::config {

using TomlNode = toml::node_view<const toml::node>;

#define ROQ_CONFIG_THROW_BAD_NODE(node, what) throw roq::RuntimeError("{}:{}", node_path(node), what);

class TomlFile {
public:    
    using Node = TomlNode;
    using Document = toml::table;
    //using ParametersHash = Hash<std::string_view, Hash<std::string_view, ParameterInfo >>;

    //TomlConfig(IdentStrings& markets, IdentStrings& portfolios)
    //: markets_(&markets)
    //, portfolios_(&portfolios) {}

    /*TomlConfig(toml::table& root, IdentStrings& markets)
    : root_(&root)
    , markets_(&markets) {}*/
    
    TomlFile() = default;

    void parse_file(std::string_view path);

    TomlFile(std::string_view path) {
        parse_file(path);
    }

    TomlFile(toml::table&& root, std::string path) 
    : root_(std::move(root))
    , path_(std::move(path)) {}

    //template<class Context>
    //void set_context(Context& context) {
    //    this->get_market_ident = [&context](std::string_view market) -> MarketIdent { return context.get_market_ident(market); };
    //    this->get_portfolio_ident = [&context](std::string_view folio) -> PortfolioIdent { return context.get_portfolio_ident(folio); };
    //}

    Node get_root() const { return TomlNode(root_); }
    void set_root(toml::table&& root) { root_ = std::move(root); }
    
    const std::string& get_path() const { return path_; }

    template<class T>
    auto operator[](T index) const { return get_root()[index]; }
    
    decltype(auto) get_string(std::string_view path) const { return get_string(get_root(), path); }
    std::string get_string(Node node, std::string_view path) const;
    std::string get_string_or(Node node, std::string default_value) const;
    std::string get_string_or(Node node, std::string_view path, std::string default_value) const;

    template<class V>
    void operator()(V& value, Node node, std::string_view path) const {
        value = get_value_or(node, path, value);
    }
    
    // extract attribute
    template<class V> 
    auto get_value(Node node, std::string_view path) const;

    template<class T>
    auto get_value_or(Node node, std::string_view path, T&& default_value) const;
    
    // extract value from node, or throw
    template<class T>
    auto get_value(Node node) const;

    // extract value from node, or return default
    template<class T>
    auto get_value_or(Node node, T&& default_value={}) const -> std::decay_t<T>;

    // enumerate child nodes
    void get_nodes(Node node, std::string_view path, std::function<void(Node)> fn) const;
    
    void get_nodes(Node node, std::function<void(Node)> fn) const;
    
    std::size_t  get_size(Node node) const;

    
    // enumerate child nodes in root node
    void get_nodes(std::string_view path, std::function<void(Node)> fn) const { get_nodes(get_root(), path, std::move(fn)); }

    // enumerate fields in node
    void get_fields(TomlNode parent, std::function<void(std::string_view key, Node val)> fn) const;

    // association
    template<class V, class Fn>
    std::size_t get_pairs(type_c<V>, Node parent, std::string_view key, std::string_view value, Fn&& fn) const;

    // scalar or array
    template<class V, class Fn>
    std::size_t get_values(type_c<V>, Node parent, std::string_view value, Fn&& fn) const;


    //void configure(roq::core::IModel& model, Node node) const;
    //void operator()(roq::core::IModel& model) const;

    std::string node_path(TomlNode node) const;

    /*
    template<class Context, class Fn>
    void get_markets(Context& context, Fn&& fn) {
        get_nodes("market", [&](Node node) {
            auto market_str = get_string(node, "market");
            auto market = context.get_market_ident(market_str);
            fn(market, node);
            log::debug("market {}", context.prn(market));
        });
    }*/
private:

    template<class T>
    auto get_value_helper(TomlNode node) const;

public:
    //std::function<MarketIdent(std::string_view)> get_market_ident;
    //std::function<PortfolioIdent(std::string_view)> get_portfolio_ident;
private:
    toml::table root_;
    std::string path_;
};



template<class T>
auto TomlFile::get_value_helper(TomlNode node) const {
    if(node.is_value()) {
        auto opt_value =  node.template value<T>();
        if(opt_value.has_value()) {
            return opt_value.value();
        } else {
            ROQ_CONFIG_THROW_BAD_NODE(node, "<empty>");
        }
    } else {
        ROQ_CONFIG_THROW_BAD_NODE(node, "string of number expected");
    }
}


template<class T>
auto TomlFile::get_value_or(TomlNode node, T&& default_value) const -> std::decay_t<T> {
    if(node)
        return get_value<std::decay_t<T>>(node);
    else 
        return default_value;
}

template<class T>
auto TomlFile::get_value_or(TomlNode node, std::string_view path,  T&& default_value) const {
    return get_value_or(node.at_path(path), std::forward<T>(default_value));
}


template<class T>
auto TomlFile::get_value(TomlNode parent, std::string_view path) const {
    if(!parent[path]) {
        ROQ_CONFIG_THROW_BAD_NODE(parent, path);
    } else {
        return get_value<T>(parent[path]);
    }
}

template<class T>
auto TomlFile::get_value(TomlNode node) const {
    if constexpr(
            std::is_same_v<roq::core::Double, T> || 
            std::is_same_v<roq::core::Price, T> ||
            std::is_same_v<roq::core::Volume, T> ) 
    {
        return T(get_value_helper<typename T::value_type>(node));
    }
    /* else if constexpr(std::is_same_v<roq::core::PortfolioIdent, T>) {
        return get_portfolio_ident(get_value_helper<std::string>(node));
    } else  if constexpr(std::is_same_v<roq::core::MarketIdent, T>) {
        return get_market_ident(get_value_helper<std::string>(node));
    }*/
    else  if constexpr(std::is_same_v<roq::core::String, T>) {
        return get_value_helper<std::string>(node);
    } else if constexpr(std::is_same_v<roq::core::Integer, T>) {
        return T(get_value_helper<int64_t>(node));
    } else if constexpr(std::is_same_v<roq::core::Bool, T>) {
        return T(get_value_helper<bool>(node));
    } else  if constexpr(std::is_enum_v<T>){ 
        std::string str =  get_value_helper<std::string>(node);
        auto value_opt = magic_enum::enum_cast<T>(str);
        return value_opt.value();
    } else {
        static_assert(!sizeof(T), "get_value<T> only supports Double, Price, Volume, Bool, String or Enums");
    }
}


template<class V, class Fn>
inline std::size_t TomlFile::get_pairs(type_c<V>, Node parent, std::string_view key, std::string_view value, Fn&& fn) const {
    auto values_node = parent[value];
    auto keys_node = parent[key];
    if(!values_node) {
      return 0;
    }
    if(!keys_node) {
        fn(std::string{}, get_value<V>(values_node));
        return 1;
    }
    std::size_t num_entries = 0;
    if(keys_node.is_array()) {
        const auto &keys = *keys_node.as_array();
        if(values_node.is_array()) {
            const auto &values = *values_node.as_array();
            for(std::size_t i=0;i<keys.size();i++) {
                auto key = get_value<std::string>(TomlNode{keys.get(i)});
                if(i<values.size())
                    fn(key, get_value<V>(TomlNode{values.get(i)}));
                else if(values.size()>0)
                    fn(key, get_value<V>(TomlNode{values.get(values.size()-1)}));
                num_entries++;
            }
        } else {
            for(std::size_t i=0;i<keys.size();i++) {
                auto key = get_value<std::string>(TomlNode{keys.get(i)});
                fn(key, get_value<V>(values_node));
                num_entries++;
            }
        }
    } else {
        // assume scalars
        fn(get_value<std::string>(keys_node), get_value<V>(values_node));
        num_entries++;
    }
    return num_entries;
}


template<class V, class Fn>
inline std::size_t TomlFile::get_values(type_c<V>, Node parent, std::string_view value, Fn&& fn) const {
    auto values_node = parent[value];
    if(!values_node) {
      return 0;
    }
    std::size_t num_entries = 0;
    if(values_node.is_array()) {
        const auto &values = *values_node.as_array();
        for(std::size_t i=0;i<values.size();i++) {
            fn(i, get_value<V>(TomlNode{values.get(i)}));
            num_entries++;
        }
    } else {
        // single value
        fn(0, get_value<V>(values_node));
        num_entries++;
    }
    return num_entries;
}

} // namespace umm
