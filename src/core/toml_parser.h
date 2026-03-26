#pragma once
#include <map>
#include <string>

namespace radicc {

std::map<std::string, std::string> parse_toml(const std::string& section);
// Look up a section by its `id = "..."` field and return key/value map.
std::map<std::string, std::string> parse_toml_by_id(const std::string& id);
std::map<std::string, std::string> parse_toml_global();
std::string get_config_path(const std::string& filename);

} // namespace radicc
