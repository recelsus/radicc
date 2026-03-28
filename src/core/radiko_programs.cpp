#include "core/radiko_programs.h"

#include "core/radiko_programs_xml.h"
#include "utils/date.h"

#include <ctime>

namespace radicc {
namespace {

bool is_basic_date8(const std::string& value) {
  if (value.size() != 8) return false;
  for (char ch : value) {
    if (ch < '0' || ch > '9') return false;
  }
  return true;
}

std::optional<ProgramEventInfo> find_program_by_station_ft_in_date_xml(
    const std::string& station_id,
    const std::string& yyyymmdd,
    const std::string& ft) {
  if (station_id.empty() || !is_basic_date8(yyyymmdd) || ft.size() != 14) return std::nullopt;
  const std::string xml = fetch_programs_xml("https://radiko.jp/v3/program/station/date/" + yyyymmdd + "/" + station_id + ".xml");
  if (xml.empty()) return std::nullopt;
  for (auto program : parse_programs_from_xml(xml)) {
    if (program.ft == ft) {
      fill_program_image_from_event_page(program);
      return program;
    }
  }
  return std::nullopt;
}

}  // namespace

std::optional<ProgramEventInfo> find_program_by_station_ft(
    const std::string& station_id,
    const std::string& ft) {
  if (station_id.empty() || ft.size() != 14) return std::nullopt;

  const std::string ft_date = ft.substr(0, 8);
  if (auto info = find_program_by_station_ft_in_date_xml(station_id, ft_date, ft)) return info;

  const std::string previous_date = shift_date8(ft_date, -1);
  if (!previous_date.empty()) {
    if (auto info = find_program_by_station_ft_in_date_xml(station_id, previous_date, ft)) return info;
  }

  const std::string xml = fetch_programs_xml("https://radiko.jp/v3/program/station/weekly/" + station_id + ".xml");
  if (xml.empty()) return std::nullopt;
  for (auto program : parse_programs_from_xml(xml)) {
    if (program.ft == ft) {
      fill_program_image_from_event_page(program);
      return program;
    }
  }
  return std::nullopt;
}

std::optional<ProgramEventInfo> find_nearest_weekly_program_info(
    const std::string& station_id,
    const std::string& title) {
  if (station_id.empty() || title.empty()) return std::nullopt;
  const std::string xml = fetch_programs_xml("https://radiko.jp/v3/program/station/weekly/" + station_id + ".xml");
  if (xml.empty()) return std::nullopt;

  auto programs = parse_programs_from_xml(xml);
  auto now = []() {
    char buf[16];
    std::time_t t = std::time(nullptr);
    std::tm* lt = std::localtime(&t);
    std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", lt);
    return std::string(buf);
  }();

  ProgramEventInfo* best = nullptr;
  for (auto& program : programs) {
    if (program.title == title && program.to <= now && (!best || program.to > best->to)) {
      best = &program;
    }
  }
  if (!best) return std::nullopt;
  fill_program_image_from_event_page(*best);
  return *best;
}

}  // namespace radicc
