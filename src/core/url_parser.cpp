#include "core/url_parser.h"
#include <optional>
#include <regex>
#include <string>
#include <tuple>

namespace radicc {

std::optional<std::tuple<std::string, std::array<std::string, 3>>>
parse_radiko_url(const std::string& url) {
  std::regex url_pattern(R"(https://radiko\.jp/#!/ts/([^/]+)/(\d{14}))");
  std::smatch match;
  if (std::regex_search(url, match, url_pattern) && match.size() == 3) {
    std::string station_id = match[1].str();
    std::string datetime = match[2].str();
    std::array<std::string, 3> dt = { datetime.substr(0, 4), datetime.substr(4, 4), datetime.substr(8) };
    return std::make_tuple(station_id, dt);
  }
  return std::nullopt;
}

} // namespace radicc

