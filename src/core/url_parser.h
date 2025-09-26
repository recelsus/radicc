#pragma once
#include <array>
#include <optional>
#include <string>
#include <tuple>

namespace radicc {

std::optional<std::tuple<std::string, std::array<std::string, 3>>>
parse_radiko_url(const std::string& url);

} // namespace radicc

