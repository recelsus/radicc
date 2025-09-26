#pragma once
#include <map>
#include <string>

namespace radicc {

std::map<std::string, std::string> parse_toml(const std::string& section);
// Look up a section by its `id = "..."` field and return key/value map.
std::map<std::string, std::string> parse_toml_by_id(const std::string& id);
std::string get_config_path(const std::string& filename);
std::string get_latest_date_for_day(const std::string& day);
std::string replace_placeholders(const std::string& templateStr, const std::string& targetDate, const std::string& time);

} // namespace radicc
