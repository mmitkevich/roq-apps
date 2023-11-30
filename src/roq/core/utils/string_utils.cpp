#include "string_utils.hpp" 
#include <algorithm>
#include <cctype>
#include <ranges>

namespace roq::core::utils {

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

} // roq::core::utils