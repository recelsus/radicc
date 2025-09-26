#pragma once
#include <array>
#include <string>

namespace radicc {

std::string generate_14digit_datetime(const std::array<std::string, 3>& datetime, int duration);

} // namespace radicc

