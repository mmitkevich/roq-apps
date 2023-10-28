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

#include <toml++/toml.h>
//#include <type_traits>
//#include <magic_enum.hpp>

namespace roq::config {

using TomlNode = toml::node_view<const toml::node>;

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

    TomlFile(std::string_view path)
    : root_(toml::parse_file(path))
    , path_(std::move(path)) 
    {}

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
    void get_pairs(type_c<V>, Node parent, std::string_view key, std::string_view value, Fn&& fn) const;

    // scalar or array
    template<class V, class Fn>
    void get_values(type_c<V>, Node parent, std::string_view value, Fn&& fn) const;


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

    [[noreturn]]
    void throw_bad_node(TomlNode node, std::string_view what) const;

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
    auto opt_value =  node.template value<T>();
    if(opt_value.has_value()) {
        return opt_value.value();
    }
    throw_bad_node(node, "<empty>");
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
        throw_bad_node(parent, path);
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
inline void TomlFile::get_pairs(type_c<V>, Node parent, std::string_view key, std::string_view value, Fn&& fn) const {
    auto values_node = parent[value];
    auto keys_node = parent[key];
    if(!values_node || !keys_node) {
      return;
    }
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
            }
        } else {
            for(std::size_t i=0;i<keys.size();i++) {
                auto key = get_value<std::string>(TomlNode{keys.get(i)});
                fn(key, get_value<V>(values_node));
            }
        }
    } else {
        // assume scalars
        fn(get_value<std::string>(keys_node), get_value<V>(values_node));
    }
}


template<class V, class Fn>
inline void TomlFile::get_values(type_c<V>, Node parent, std::string_view value, Fn&& fn) const {
    auto values_node = parent[value];
    if(!values_node) {
      return;
    }
    if(values_node.is_array()) {
        const auto &values = *values_node.as_array();
        for(std::size_t i=0;i<values.size();i++) {
            fn(i, get_value<V>(TomlNode{values.get(i)}));
        }
    } else {
        // single value
        fn(0, get_value<V>(values_node));
    }
}


inline void TomlFile::get_nodes(TomlNode parent, std::string_view path, std::function<void(TomlNode)> fn) const {
    auto node = parent.at_path(path);
    return get_nodes(node, fn);
}

inline std::size_t  TomlFile::get_size(TomlNode node) const {
    if(!node)
        return 0;
    if(!node.is_array())
        return 0;
    return node.as_array()->size();
}

inline void TomlFile::get_nodes(TomlNode node, std::function<void(TomlNode)> fn) const {
    std::size_t i=0;
    if(node) {
        if(node.is_array()) {
            for(auto const& subnode: *node.as_array())
                fn(TomlNode{subnode});
        } else {
            fn(node);
        }
    }
}

inline void TomlFile::get_fields(TomlNode parent, std::function<void(std::string_view key, TomlNode val)> fn) const {
    if(parent.is_table()) {
        for(auto const& kv: *parent.as_table()) {
            fn(kv.first.str(), TomlNode(kv.second));
        }
    }
}

inline  std::string TomlFile::get_string(TomlNode parent, std::string_view path) const  {
    auto node = parent.at_path(path);
    if(node) {
        return get_value_helper<std::string>(node);
    }
    throw_bad_node(parent, path);
}

inline std::string TomlFile::get_string_or(Node node, std::string default_value) const {
    if(node)
        return get_value_helper<std::string>(node);
    return default_value;
}

inline std::string TomlFile::get_string_or(Node parent, std::string_view path, std::string default_value) const {
    auto node = parent.at_path(path);
    if(node)
        return get_value_helper<std::string>(node);
    return default_value;
}


inline std::string TomlFile::node_path(TomlNode node) const {
  if (!node)
    return "";
  const auto &src = node.node()->source();
  // TODO: return fmt::format("{}:{}:{}"sv, src.path?*src.path:"",src.begin.line,src.begin.column)
  std::stringstream ss;
  if (src.path != nullptr) {
    ss << *src.path;
  }
  ss << ":"<<src.begin.line << ":" << src.begin.column;
  return ss.str();
}

inline void TomlFile::throw_bad_node(TomlNode node, std::string_view what) const {
  throw roq::RuntimeError("{}:{}", node_path(node), what);
}


} // namespace umm
