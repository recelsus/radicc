#pragma once

#include <array>
#include <ctime>
#include <string>

namespace radicc {

[[noreturn]] void print_error_and_exit(const std::string& message);
bool has_datetime_components(const std::array<std::string, 3>& datetime);
std::time_t to_time_utc_like(const std::string& yyyymmddhhmmss);
int diff_minutes(const std::string& ft, const std::string& to);
std::string json_escape(const std::string& input);
std::string current_yyyymmdd_jst();
bool is_valid_date8(const std::string& value);
std::string time_hhmm(const std::string& datetime14);
std::string build_timefree_url(const std::string& station_id, const std::string& ft);

}  // namespace radicc
