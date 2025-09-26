#pragma once
#include <optional>
#include <string>

namespace radicc {

// Fetches program URL for given station and date by matching exact title.
// Returns mobile events URL like: https://radiko.jp/mobile/events/<programId>
std::optional<std::string> find_program_event_url(
    const std::string& station_id,
    const std::string& yyyymmdd,
    const std::string& title);

// Extended info: returns pair (event_url, image_url). Either may be empty.
struct ProgramEventInfo {
  std::string event_url;
  std::string image_url;
  std::string ft;   // start datetime yyyymmddHHMMSS
  std::string to;   // end   datetime yyyymmddHHMMSS
  std::string pfm;  // performer/personality
  std::string img;  // image from XML if present
  std::string title; // program title (from XML)
};
std::optional<ProgramEventInfo> find_program_event_info(
    const std::string& station_id,
    const std::string& yyyymmdd,
    const std::string& title);

// Nearest program from weekly XML by exact title, preferring past programs (to <= now).
std::optional<ProgramEventInfo> find_nearest_weekly_program_info(
    const std::string& station_id,
    const std::string& title);

// Exact match by station_id and ft on weekly XML.
std::optional<ProgramEventInfo> find_program_by_station_ft(
    const std::string& station_id,
    const std::string& ft);

} // namespace radicc
