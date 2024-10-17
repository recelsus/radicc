#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <optional>
#include <string>
#include <array>
#include <tuple>

std::optional<std::tuple<std::string, std::array<std::string, 3>>>
parseRadikoURL(const std::string& url);

#endif
