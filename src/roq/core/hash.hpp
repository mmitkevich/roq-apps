#pragma once

#include <absl/container/flat_hash_map.h>
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

template<class K, class V, class...Args>
struct Hash : absl::flat_hash_map<K, V, Args...> 
{
    using Base = absl::flat_hash_map<K, V, Args...> ;
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

template<class K, class V, class...Args>
struct UHash : absl::flat_hash_map<K, std::unique_ptr<V>, Args...> 
{
    using Base = absl::flat_hash_map<K, std::unique_ptr<V>, Args...> ;
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




} // roq::core