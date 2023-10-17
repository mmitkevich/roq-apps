#pragma once

#include <absl/container/flat_hash_map.h>
namespace roq::core {

template<class K, class V, class...Args>
struct Hash : absl::flat_hash_map<K, V, Args...> 
{
    using Base = absl::flat_hash_map<K, V, Args...> ;
    using Base::Base;

    template<class KK>
    V operator()(const KK& key, V&& dflt = {}) const {
        auto iter = this->find(key);
        return iter!=this->end() ? iter->second : std::move(dflt);
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
};


} // roq::core