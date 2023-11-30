#pragma once

#include <string_view>
#include <vector>
#include <string>

namespace roq::core::utils {

template<typename T, std::enable_if_t< std::is_same<T, std::string_view>::value || !std::is_rvalue_reference_v<T&&>, int > = 0 >
std::string_view trim_left( T&& data, std::string_view trimChars )
{
    std::string_view sv{std::forward<T>(data)};
    sv.remove_prefix( std::min(sv.find_first_not_of(trimChars), sv.size()));
    return sv;
}

std::string to_upper(std::string_view s);

std::pair<std::string_view, std::string_view>
split_prefix(std::string_view input, char sep);

std::pair<std::string_view, std::string_view>
split_suffix(std::string_view input, char sep);

std::vector<std::string_view> split_sep(std::string_view str, char sep);

} // roq::core::utils