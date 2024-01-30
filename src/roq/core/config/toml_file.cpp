#include "toml_file.hpp" 

namespace roq::core::config {

void TomlFile::parse_file(std::string_view path) {
    try {
        root_ = toml::parse_file(path);
    } catch (const toml::parse_error &err) {
        throw roq::RuntimeError("error: {}:{}: {}", *err.source().path, err.source().begin.line, err.description());
    }
    path_ = path;
}



void TomlFile::get_nodes(TomlNode parent, std::string_view path, std::function<void(TomlNode)> fn) const {
    auto node = parent.at_path(path);
    return get_nodes(node, fn);
}

std::size_t  TomlFile::get_size(TomlNode node) const {
    if(!node)
        return 0;
    if(!node.is_array())
        return 0;
    return node.as_array()->size();
}

void TomlFile::get_nodes(TomlNode node, std::function<void(TomlNode)> fn) const {
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

void TomlFile::get_fields(TomlNode parent, std::function<void(std::string_view key, TomlNode val)> fn) const {
    if(parent.is_table()) {
        for(auto const& kv: *parent.as_table()) {
            fn(kv.first.str(), TomlNode(kv.second));
        }
    }
}

std::string TomlFile::get_string(TomlNode parent, std::string_view path) const  {
    auto node = parent.at_path(path);
    if(node) {
        return get_value_helper<std::string>(node);
    }
    ROQ_CONFIG_THROW_BAD_NODE(parent, path);
}

std::string TomlFile::get_string_or(Node node, std::string default_value) const {
    if(node)
        return get_value_helper<std::string>(node);
    return default_value;
}

std::string TomlFile::get_string_or(Node parent, std::string_view path, std::string default_value) const {
    auto node = parent.at_path(path);
    if(node)
        return get_value_helper<std::string>(node);
    return default_value;
}


std::string TomlFile::node_path(TomlNode node) const {
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

} // roq::config