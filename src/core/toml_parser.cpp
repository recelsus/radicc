#include "core/toml_parser.h"
#include "toml.hpp"
#include <iostream>
#include <map>
#include <sys/stat.h>

namespace radicc {

namespace {

std::map<std::string, std::string> extract_string_map(const toml::table& table) {
  std::map<std::string, std::string> result;
  for (const auto& [key, value] : table) {
    if (const auto* str_value = value.as_string()) {
      result.emplace(std::string(key), str_value->get());
    } else if (const auto* int_value = value.as_integer()) {
      result.emplace(std::string(key), std::to_string(int_value->get()));
    }
  }
  return result;
}

}  // namespace

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
    if (auto* sec = config[section].as_table()) {
      std::map<std::string, std::string> result = extract_string_map(*sec);
      for (const auto& [key, value] : *sec) {
        if (!value.is_string() && !value.is_integer()) {
          std::cerr << "Unsupported value type for key: " << key << std::endl;
        }
      }
      return result;
    } else {
      std::cerr << "Warning: Section '" << section << "' not found in TOML file.\n";
      return {};
    }
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
          std::map<std::string, std::string> result = extract_string_map(*sec);
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

std::map<std::string, std::string> parse_toml_global() {
  try {
    const std::string toml_file_path = get_config_path("radicc.toml");
    if (toml_file_path.empty() || !std::ifstream(toml_file_path)) {
      return {};
    }
    auto config = toml::parse_file(toml_file_path);
    if (!config.is_table()) return {};

    std::map<std::string, std::string> result;
    if (const auto* base_dir = config["base_dir"].as_string()) {
      result.emplace("base_dir", base_dir->get());
    }
    return result;
  } catch (const toml::parse_error& err) {
    std::cerr << "Error parsing TOML file: " << err.what() << std::endl;
    return {};
  }
}

} // namespace radicc
