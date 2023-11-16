#pragma once
#include <string_view>

namespace roq::core {

template<typename T, std::enable_if_t< std::is_same<T, std::string_view>::value || !std::is_rvalue_reference_v<T&&>, int > = 0 >
std::string_view trim_left( T&& data, std::string_view trimChars )
{
    std::string_view sv{std::forward<T>(data)};
    sv.remove_prefix( std::min(sv.find_first_not_of(trimChars), sv.size()));
    return sv;
}    

}