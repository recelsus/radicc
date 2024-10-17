#include "toml_parser.h"
#include "toml.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <regex>
#include <stdexcept>
#include <sys/stat.h>

std::string getConfigPath(const std::string& filename) {
    const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
    std::string config_path;

    if (xdg_config_home) {
        config_path = std::string(xdg_config_home) + "/radicc";
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            config_path = std::string(home) + "/.config/radicc";
        } else {
            std::cerr << "Error: Could not determine home directory.\n";
            std::exit(1);
        }
    }

    struct stat st;
    if (stat(config_path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        std::cerr << "Error: Config directory does not exist: " << config_path << "\n";
        return "";
    }

    return config_path + "/" + filename;
}

std::string getLatestDateForDay(const std::string& day) {
    std::map<std::string, int> dayMap = {
        {"Sunday", 0}, {"Monday", 1}, {"Tuesday", 2},
        {"Wednesday", 3}, {"Thursday", 4}, {"Friday", 5}, {"Saturday", 6}
    };

    if (dayMap.find(day) == dayMap.end()) {
        throw std::invalid_argument("Invalid day: " + day);
    }

    std::time_t now = std::time(nullptr);
    std::tm* t = std::localtime(&now);
    int today = t->tm_wday;

    int daysBack = (today - dayMap[day] + 7) % 7;
    if (daysBack == 0) daysBack = 7;

    t->tm_mday -= daysBack;
    std::mktime(t);

    std::ostringstream os;
    os << std::put_time(t, "%Y%m%d");
    return os.str();
}

std::string replacePlaceholders(const std::string& templateStr, const std::string& targetDate, const std::string& time) {
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

        std::ostringstream os;
        os << std::put_time(&t, "%Y%m%d");
        result = std::regex_replace(result, offsetRegex, os.str());
    }

    return result;
}

std::map<std::string, std::string> parseTOML(const std::string& section) {
    try {
        const std::string TOML_FILE_PATH = getConfigPath("radicc.toml");

        if (TOML_FILE_PATH.empty() || !std::ifstream(TOML_FILE_PATH)) {
            std::cerr << "Warning: TOML file not found. Proceeding with command-line arguments.\n";
            return {};
        }

        std::ifstream toml_file(TOML_FILE_PATH);
        if (!toml_file) {
            std::cerr << "Error: Could not open TOML file at " << TOML_FILE_PATH << "\n";
            return {};
        }

        auto config = toml::parse_file(TOML_FILE_PATH);
        std::map<std::string, std::string> result;

        if (auto* sec = config[section].as_table()) {
            for (const auto& [key, value] : *sec) {
                if (const auto* strValue = value.as_string()) {
                    result.emplace(std::string(key), strValue->get());
                } else if (const auto* intValue = value.as_integer()) {
                    result.emplace(std::string(key), std::to_string(intValue->get()));
                } else {
                    std::cerr << "Unsupported value type for key: " << key << std::endl;
                }
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

