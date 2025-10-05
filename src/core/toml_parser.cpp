#include "core/toml_parser.h"
#include "toml.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>

namespace radicc {

std::string get_config_path(const std::string& filename) {
  const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
  std::string config_path;
  if (xdg_config_home) config_path = std::string(xdg_config_home) + "/radicc";
  else {
    const char* home = std::getenv("HOME");
    if (home) config_path = std::string(home) + "/.config/radicc";
    else { std::cerr << "Error: Could not determine home directory.\n"; std::exit(1); }
  }
  struct stat st;
  if (stat(config_path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
    std::cerr << "Error: Config directory does not exist: " << config_path << "\n";
    return "";
  }
  return config_path + "/" + filename;
}

static std::string to_lower(const std::string& s) {
  std::string out = s; std::transform(out.begin(), out.end(), out.begin(), ::tolower); return out;
}

std::string get_latest_date_for_day(const std::string& day) {
  std::map<std::string, int> dayMap = {
      {"sunday", 0}, {"monday", 1}, {"tuesday", 2},
      {"wednesday", 3}, {"thursday", 4}, {"friday", 5}, {"saturday", 6},
      {"sun", 0}, {"mon", 1}, {"tue", 2}, {"wed", 3}, {"thu", 4}, {"fri", 5}, {"sat", 6}
  };
  std::string lowerDay = to_lower(day);
  if (dayMap.find(lowerDay) == dayMap.end()) throw std::invalid_argument("Invalid day: " + day);
  std::time_t now = std::time(nullptr);
  std::tm* t = std::localtime(&now);
  int today = t->tm_wday;
  int daysBack = (today - dayMap[lowerDay] + 7) % 7;
  if (daysBack == 0) daysBack = 7;
  t->tm_mday -= daysBack;
  std::mktime(t);
  std::ostringstream os; os << std::put_time(t, "%Y%m%d");
  return os.str();
}

std::string replace_placeholders(const std::string& templateStr, const std::string& targetDate, const std::string& time) {
  std::string result = templateStr;
  result = std::regex_replace(result, std::regex("\\{yyyyMMdd\\}"), targetDate);
  result = std::regex_replace(result, std::regex("\\{HHmm\\}"), time);
  std::regex offsetRegex(R"(\{yyyyMMdd, (\d+)\})");
  std::smatch match;
  if (std::regex_search(result, match, offsetRegex)) {
    int offset = std::stoi(match[1].str());
    std::tm t = {};
    std::istringstream ss(targetDate);
    ss >> std::get_time(&t, "%Y%m%d");
    t.tm_mday -= offset;
    std::mktime(&t);
    std::ostringstream os; os << std::put_time(&t, "%Y%m%d");
    result = std::regex_replace(result, offsetRegex, os.str());
  }
  return result;
}

std::map<std::string, std::string> parse_toml(const std::string& section) {
  try {
    const std::string toml_file_path = get_config_path("radicc.toml");
    if (toml_file_path.empty() || !std::ifstream(toml_file_path)) {
      std::cerr << "Warning: TOML file not found. Proceeding with command-line arguments.\n";
      return {};
    }
    std::ifstream toml_file(toml_file_path);
    if (!toml_file) { std::cerr << "Error: Could not open TOML file at " << toml_file_path << "\n"; return {}; }
    auto config = toml::parse_file(toml_file_path);
    std::map<std::string, std::string> result;
    if (auto* sec = config[section].as_table()) {
      for (const auto& [key, value] : *sec) {
        if (const auto* strValue = value.as_string())      result.emplace(std::string(key), strValue->get());
        else if (const auto* intValue = value.as_integer()) result.emplace(std::string(key), std::to_string(intValue->get()));
        else std::cerr << "Unsupported value type for key: " << key << std::endl;
      }
    } else {
      std::cerr << "Warning: Section '" << section << "' not found in TOML file.\n";
      return {};
    }
    return result;
  } catch (const toml::parse_error& err) {
    std::cerr << "Error parsing TOML file: " << err.what() << std::endl;
    return {};
  }
}

std::map<std::string, std::string> parse_toml_by_id(const std::string& id) {
  try {
    const std::string toml_file_path = get_config_path("radicc.toml");
    if (toml_file_path.empty() || !std::ifstream(toml_file_path)) {
      std::cerr << "Warning: TOML file not found.\n";
      return {};
    }
    auto config = toml::parse_file(toml_file_path);
    if (!config.is_table()) return {};
    const auto* root = config.as_table();
    for (const auto& [sec_name, node] : *root) {
      if (auto sec = node.as_table()) {
        const auto* idv = (*sec)["id"].as_string();
        if (idv && idv->get() == id) {
          std::map<std::string, std::string> result;
          for (const auto& [key, value] : *sec) {
            if (const auto* sv = value.as_string()) result.emplace(std::string(key), sv->get());
            else if (const auto* iv = value.as_integer()) result.emplace(std::string(key), std::to_string(iv->get()));
          }
          if (!result.count("title")) result.emplace("title", sec_name);
          return result;
        }
      }
    }
    return {};
  } catch (const toml::parse_error& err) {
    std::cerr << "Error parsing TOML file: " << err.what() << std::endl;
    return {};
  }
}

} // namespace radicc
