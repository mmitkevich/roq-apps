// (c) copyright 2023 Mikhail Mitkevich
#pragma once

//#include <absl/container/flat_hash_map.h>
//#include <absl/container/node_hash_map.h>

#include <roq/utils/container.hpp>
#include <map>


namespace ankerl::unordered_dense {

template<typename T> 
struct hash<T, std::enable_if_t<
std::is_assignable_v<std::string_view, const T&> && !std::is_same_v<std::string_view, T>  && !std::is_same_v<std::string, T>
>>  {
  using is_transparent = void;  // heterogeneous overloads
  using is_avalanching = void;  // high quality avalanching hash

  [[nodiscard]] auto operator()(T const &key) const noexcept {
    std::string_view s = key;
    return ankerl::unordered_dense::hash<std::string_view>{}(s);
  }
};


} // ankerl::unordered_dense

namespace roq {

//template<std::size_t N>
//constexpr auto operator==( String<N> const& lhs, std::string_view const &rhs) {
//    return std::string_view{lhs} == rhs;
//}

}// roq

namespace roq::core {

template<class Hash, class Key, class Fn>
bool hash_get_value(Hash & hash, Key const& key, Fn&& fn) {
    auto iter = hash.find(key);
    if(iter!=std::end(hash)) {
        fn(iter->second);
        return true;
    }
    return false;
}

template<class K, class V>
struct Hash : 
roq::utils::unordered_map<K,V>
//absl::flat_hash_map<K, V, Args...> 
{
    using Base = //absl::flat_hash_map<K, V, Args...> ;
    roq::utils::unordered_map<K,V>;
    using Base::Base;

    template<class Key>
    V operator()(const Key& key, V&& dflt = {}) const {
        auto iter = this->find(key);
        return iter!=this->end() ? iter->second : std::move(dflt);
    }
    
    template<class Key, class Fn>
    bool get_value(Key const& key, Fn&& fn) {
        return core::hash_get_value(*this, key, std::move(fn));
    }
    template<class Key, class Fn>
    bool get_value(Key const& key, Fn&& fn) const {
        return core::hash_get_value(*this, key, std::move(fn));
    }
};

template<class K, class V>
struct UHash : 
roq::utils::unordered_map<K, std::unique_ptr<V>>
//absl::flat_hash_map<K, std::unique_ptr<V>> 
{
    using Base = 
        roq::utils::unordered_map<K, std::unique_ptr<V>>;
        //absl::flat_hash_map<K, std::unique_ptr<V>> ;
    using Base::Base;

    using Base::find;
    
    template<class KK>
    V operator()(const KK& key, V&& dflt = {}) const {
        auto iter = this->find(key);
        return iter!=this->end() ? *iter->second : std::move(dflt);
    }
        
    template<class Key, class Fn>
    bool get_value(Key const& key, Fn&& fn) {
        return core::hash_get_value(*this, key, std::move(fn));
    }
    template<class Key, class Fn>
    bool get_value(Key const& key, Fn&& fn) const {
        return core::hash_get_value(*this, key, std::move(fn));
    }    
};

template<class K, class V, class...Args> 
struct Map : 
std::map<K, V, Args...>
//absl::node_hash_map <K, V> 
{
    using Base = 
        std::map<K, V, Args...>;
        //absl::node_hash_map <K, V>;
    using Base::Base;
};

} // roq::core