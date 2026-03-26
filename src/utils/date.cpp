#include "utils/date.h"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <sstream>
#include <stdexcept>

namespace radicc {

namespace {

bool is_leap_year(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int days_in_month(int year, int month) {
  static constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && is_leap_year(year)) return 29;
  return kDays[month - 1];
}

}  // namespace

std::string generate_14digit_datetime(const std::array<std::string, 3>& datetime, int duration) {
  std::string yyyy = datetime[0];
  std::string MMdd = datetime[1];
  std::string HHmmss = datetime[2];
  std::tm t = {};
  std::istringstream ss(yyyy + MMdd + HHmmss.substr(0, 4));
  ss >> std::get_time(&t, "%Y%m%d%H%M");
  if (ss.fail()) throw std::invalid_argument("Invalid datetime format.");
  t.tm_min += duration;
  std::mktime(&t);
  std::ostringstream os;
  os << std::setw(4) << std::setfill('0') << t.tm_year + 1900
     << std::setw(2) << std::setfill('0') << t.tm_mon + 1
     << std::setw(2) << std::setfill('0') << t.tm_mday
     << std::setw(2) << std::setfill('0') << t.tm_hour
     << std::setw(2) << std::setfill('0') << t.tm_min;
  std::string result = os.str();
  return (result.size() == 12) ? result + "00" : result;
}

bool is_valid_datetime14(const std::string& value) {
  if (value.size() != 14) return false;
  for (char ch : value) {
    if (ch < '0' || ch > '9') return false;
  }

  const int year = std::stoi(value.substr(0, 4));
  const int month = std::stoi(value.substr(4, 2));
  const int day = std::stoi(value.substr(6, 2));
  const int hour = std::stoi(value.substr(8, 2));
  const int minute = std::stoi(value.substr(10, 2));
  const int second = std::stoi(value.substr(12, 2));

  if (year < 1970) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > days_in_month(year, month)) return false;
  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;
  if (second < 0 || second > 59) return false;
  return true;
}

std::int64_t to_unixtime_jst(const std::string& datetime14) {
  if (!is_valid_datetime14(datetime14)) return -1;

  int year = std::stoi(datetime14.substr(0, 4));
  int month = std::stoi(datetime14.substr(4, 2));
  const int day = std::stoi(datetime14.substr(6, 2));
  const int hour = std::stoi(datetime14.substr(8, 2));
  const int minute = std::stoi(datetime14.substr(10, 2));
  const int second = std::stoi(datetime14.substr(12, 2));

  if (month < 3) {
    month += 12;
    --year;
  }

  constexpr std::int64_t kJstOffset = 32400;
  const std::int64_t days = (365LL * year) + (year / 4) - (year / 100) + (year / 400)
      + ((306LL * (month + 1)) / 10) - 428 + day - 719163;
  return days * 86400 + (hour * 3600) + (minute * 60) + second - kJstOffset;
}

std::string from_unixtime_jst(std::int64_t unixtime) {
  constexpr std::int64_t kJstOffset = 32400;
  std::int64_t tm = unixtime + kJstOffset;
  if (tm < 0) return {};

  const int second = static_cast<int>(tm % 60);
  tm /= 60;
  const int minute = static_cast<int>(tm % 60);
  tm /= 60;
  const int hour = static_cast<int>(tm % 24);
  std::int64_t left_days = tm / 24 + 1;

  int year = 1970;
  while (left_days > 0) {
    const int year_days = is_leap_year(year) ? 366 : 365;
    if (left_days > year_days) {
      left_days -= year_days;
      ++year;
      continue;
    }
    break;
  }

  int month = 1;
  while (month <= 12) {
    const int dim = days_in_month(year, month);
    if (left_days > dim) {
      left_days -= dim;
      ++month;
      continue;
    }
    break;
  }

  const int day = static_cast<int>(left_days);
  std::ostringstream os;
  os << std::setw(4) << std::setfill('0') << year
     << std::setw(2) << std::setfill('0') << month
     << std::setw(2) << std::setfill('0') << day
     << std::setw(2) << std::setfill('0') << hour
     << std::setw(2) << std::setfill('0') << minute
     << std::setw(2) << std::setfill('0') << second;
  return os.str();
}

} // namespace radicc
