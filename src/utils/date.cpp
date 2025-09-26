#include "utils/date.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace radicc {

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

} // namespace radicc

