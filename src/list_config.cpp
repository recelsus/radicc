#include "toml_parser.h"
#include "toml.hpp"
#include <iostream>
#include <string>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>

std::string formatTimeRange(const std::string& time, int duration) {
    if (time.length() != 4) {
        return "(invalid time format)";
    }

    int hour = std::stoi(time.substr(0, 2));
    int minute = std::stoi(time.substr(2, 2));

    int endHour = hour + (minute + duration) / 60;
    int endMinute = (minute + duration) % 60;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hour << ":" 
        << std::setw(2) << minute << " - " 
        << std::setw(2) << endHour << ":" 
        << std::setw(2) << endMinute;

    return oss.str();
}

std::string formatDay(const std::string& day) {
    if (day.empty()) {
        return "(non)";
    }

    std::string formattedDay = day.substr(0, 3);
    std::transform(formattedDay.begin(), formattedDay.begin() + 1, formattedDay.begin(), ::toupper);
    std::transform(formattedDay.begin() + 1, formattedDay.end(), formattedDay.begin() + 1, ::tolower);

    return formattedDay;
}

void viewTomlConfigLists() {

    const std::string TOML_FILE_PATH = getConfigPath("radicc.toml");

    if (TOML_FILE_PATH.empty() || !std::ifstream(TOML_FILE_PATH)) {
        std::cerr << "Warning: TOML file not found. Proceeding with command-line arguments.\n";
        return;
    }

    std::ifstream toml_file(TOML_FILE_PATH);
    if (!toml_file) {
        std::cerr << "Error: Could not open TOML file at " << TOML_FILE_PATH << "\n";
        return;
    }

    auto config = toml::parse_file(TOML_FILE_PATH);

    std::cout << std::left 
              << std::setw(12) << "Station" 
              << std::setw(4) << "Day" 
              << std::setw(15) << "Time" 
              << std::setw(120) << "Section" 
              << std::endl;

    std::cout << std::string(150, '-') << std::endl;

    if (const auto* table = config.as_table()) {
        for (const auto& [section, value] : *table) {
            if (auto* sec = value.as_table()) {
                std::string day = sec->contains("day") ? (*sec)["day"].value_or("") : "";
                std::string time = sec->contains("time") ? (*sec)["time"].value_or("") : "";
                int duration = sec->contains("duration") ? (*sec)["duration"].value_or(0) : 0;
                std::string station = sec->contains("station") ? (*sec)["station"].value_or("") : "";

                std::string timeRange;
                if (time != "" && duration) {
                  timeRange = formatTimeRange(time, duration);
                }
                
                std::string weekday;
                if (day != "") {
                  weekday = formatDay(day);
                }

                std::cout << std::left 
                          << std::setw(12) << station
                          << std::setw(4) << (weekday.empty() ? day : weekday)
                          << std::setw(15) << (timeRange.empty() ? time : timeRange)
                          << std::setw(120) << section.str()
                          << std::endl;
            }
        }
    }

    std::cout << std::string(150, '-') << std::endl;
}

