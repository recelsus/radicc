#include "app/common.h"

#include <ctime>

namespace radicc {
namespace {

void append_utf8(std::string& output, unsigned int codepoint) {
  if (codepoint <= 0x7F) {
    output.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7FF) {
    output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
    output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0xFFFF) {
    output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
    output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0x10FFFF) {
    output.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
    output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
}

int hex_value(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  return -1;
}

bool append_numeric_entity(const std::string& input, std::size_t start, std::size_t& next, std::string& output) {
  if (input.compare(start, 2, "&#") != 0) return false;
  const std::size_t end = input.find(';', start + 2);
  if (end == std::string::npos) return false;

  unsigned int codepoint = 0;
  std::size_t pos = start + 2;
  const bool is_hex = pos < end && (input[pos] == 'x' || input[pos] == 'X');
  if (is_hex) ++pos;
  if (pos == end) return false;

  for (; pos < end; ++pos) {
    const int digit = is_hex ? hex_value(input[pos]) : (input[pos] >= '0' && input[pos] <= '9' ? input[pos] - '0' : -1);
    if (digit < 0) return false;
    codepoint = codepoint * (is_hex ? 16 : 10) + static_cast<unsigned int>(digit);
    if (codepoint > 0x10FFFF) return false;
  }

  append_utf8(output, codepoint);
  next = end + 1;
  return true;
}

}  // namespace

[[noreturn]] void print_error_and_exit(const std::string& message) {
  throw RadiccError(message);
}

bool has_datetime_components(const std::array<std::string, 3>& datetime) {
  return datetime[0].size() == 4 && datetime[1].size() == 4 && datetime[2].size() == 6;
}

std::time_t to_time_utc_like(const std::string& yyyymmddhhmmss) {
  if (yyyymmddhhmmss.size() != 14) return 0;
  std::tm t{};
  t.tm_isdst = -1;
  t.tm_year = std::stoi(yyyymmddhhmmss.substr(0, 4)) - 1900;
  t.tm_mon = std::stoi(yyyymmddhhmmss.substr(4, 2)) - 1;
  t.tm_mday = std::stoi(yyyymmddhhmmss.substr(6, 2));
  t.tm_hour = std::stoi(yyyymmddhhmmss.substr(8, 2));
  t.tm_min = std::stoi(yyyymmddhhmmss.substr(10, 2));
  t.tm_sec = std::stoi(yyyymmddhhmmss.substr(12, 2));
  return std::mktime(&t);
}

int diff_minutes(const std::string& ft, const std::string& to) {
  const std::time_t t_ft = to_time_utc_like(ft);
  const std::time_t t_to = to_time_utc_like(to);
  if (t_ft == 0 || t_to == 0) return 0;
  const double seconds = std::difftime(t_to, t_ft);
  return seconds > 0 ? static_cast<int>(seconds / 60) : 0;
}

std::string json_escape(const std::string& input) {
  static constexpr char hex[] = "0123456789ABCDEF";
  std::string escaped;
  escaped.reserve(input.size() + 8);
  for (unsigned char c : input) {
    switch (c) {
      case '\\': escaped += "\\\\"; break;
      case '"': escaped += "\\\""; break;
      case '\b': escaped += "\\b"; break;
      case '\f': escaped += "\\f"; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default:
        if (c < 0x20) {
          escaped += "\\u00";
          escaped.push_back(hex[c >> 4]);
          escaped.push_back(hex[c & 0x0F]);
        } else {
          escaped.push_back(static_cast<char>(c));
        }
    }
  }
  return escaped;
}

std::string decode_xml_entities(const std::string& input) {
  std::string output;
  output.reserve(input.size());
  for (std::size_t i = 0; i < input.size();) {
    if (input.compare(i, 5, "&amp;") == 0) {
      output.push_back('&');
      i += 5;
    } else if (input.compare(i, 6, "&quot;") == 0) {
      output.push_back('"');
      i += 6;
    } else if (input.compare(i, 6, "&apos;") == 0) {
      output.push_back('\'');
      i += 6;
    } else if (input.compare(i, 4, "&lt;") == 0) {
      output.push_back('<');
      i += 4;
    } else if (input.compare(i, 4, "&gt;") == 0) {
      output.push_back('>');
      i += 4;
    } else if (std::size_t next = i; append_numeric_entity(input, i, next, output)) {
      i = next;
    } else {
      output.push_back(input[i++]);
    }
  }
  return output;
}

std::string current_yyyymmdd_jst() {
  std::time_t now = std::time(nullptr) + 9 * 60 * 60;
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &now);
#else
  gmtime_r(&now, &tm);
#endif
  char buf[9];
  std::strftime(buf, sizeof(buf), "%Y%m%d", &tm);
  return std::string(buf);
}

bool is_valid_date8(const std::string& value) {
  if (value.size() != 8) return false;
  for (char ch : value) {
    if (ch < '0' || ch > '9') return false;
  }
  return true;
}

std::string time_hhmm(const std::string& datetime14) {
  return datetime14.size() >= 12 ? datetime14.substr(8, 2) + ":" + datetime14.substr(10, 2) : std::string();
}

std::string build_timefree_url(const std::string& station_id, const std::string& ft) {
  if (station_id.empty() || ft.size() != 14) return {};
  return "https://radiko.jp/#!/ts/" + station_id + "/" + ft;
}

}  // namespace radicc
