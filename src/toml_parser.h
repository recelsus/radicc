#ifndef TOML_PARSER_H
#define TOML_PARSER_H

#include <string>
#include <map>

std::map<std::string, std::string> parseTOML(const std::string& section);

std::string getConfigPath(const std::string& filename);

std::string getLatestDateForDay(const std::string& day);
std::string replacePlaceholders(const std::string& templateStr, const std::string& targetDate, const std::string& time);

#endif

