#include "url_parser.h"
#include <regex>
#include <optional>
#include <string>
#include <tuple>

std::optional<std::tuple<std::string, std::array<std::string, 3>>>
parseRadikoURL(const std::string& url) {
  std::regex urlPattern(R"(https://radiko\.jp/#!/ts/([^/]+)/(\d{14}))");
  std::smatch match;

  if (std::regex_search(url, match, urlPattern) && match.size() == 3) {
    std::string station_id = match[1].str();
    std::string datetime = match[2].str(); 

    std::string yyyy = datetime.substr(0, 4);
    std::string MMdd = datetime.substr(4, 4);
    std::string HHmmss = datetime.substr(8);

    std::array<std::string, 3> datetimeArray = { yyyy, MMdd, HHmmss };

    return std::make_tuple(station_id, datetimeArray);
  }
  return std::nullopt;
}
