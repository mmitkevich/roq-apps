#include "string_utils.hpp" 
#include <algorithm>
#include <cctype>
#include <charconv>
#include <ranges>
#include <roq/exceptions.hpp>

namespace roq::core {

std::vector < std::string_view> split_sep(std::string_view str, char sep) {
    std::vector<std::string_view> result;
    for (const auto tok : std::views::split(std::string_view{str}, sep)) {
        result.push_back(std::string_view{tok.data(), tok.size()});
    }
    return result;
}

std::pair<std::string_view, std::string_view> split_suffix(std::string_view input, char sep) {
  using namespace std::literals;
  auto pos = input.find(sep);
  if (pos == std::string_view::npos) {
    return {input, ""sv};
  }
  return {std::string_view{input.data(), pos},
          std::string_view{input.data() + pos + 1, input.size() - pos - 1}};
}

std::pair<std::string_view, std::string_view> split_prefix(std::string_view input, char sep) {
  using namespace std::literals;
  auto pos = input.find(sep);
  if (pos == std::string_view::npos) {
    return {""sv, input};
  }
  return {std::string_view{input.data(), pos},
          std::string_view{input.data() + pos + 1, input.size() - pos - 1}};
}

std::string to_upper(std::string_view s) {
  std::string r{s};
  std::transform(r.begin(), r.end(), r.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return r;
}


std::uint32_t parse_uint32(std::string_view s) {
    uint32_t v;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec == std::errc()) {
        return v;
    } else  {
        //if(ec == std::errc::invalid_argument)
        //if(ec == std::errc::result_out_of_range)
        throw roq::RuntimeError("invalid integer {}", s);
    }
    return v;
}

std::pair<std::string_view, std::uint32_t> split_suffix_uint32(std::string_view input, char sep) {
  auto [value, suffix] = split_suffix(input, sep);
  uint32_t index = suffix.size() > 0 ?  core::parse_uint32(suffix) : 0;
  return {value, index};
}

} // roq::core