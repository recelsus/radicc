#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace radicc {

std::string generate_14digit_datetime(const std::array<std::string, 3>& datetime, int duration);
bool is_valid_datetime14(const std::string& value);
std::int64_t to_unixtime_jst(const std::string& datetime14);
std::string from_unixtime_jst(std::int64_t unixtime);
std::string shift_date8(const std::string& yyyymmdd, int days);

} // namespace radicc
