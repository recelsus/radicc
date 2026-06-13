#include "core/radiko_programs.h"

#include "core/radiko_programs_xml.h"
#include "utils/date.h"

#include <algorithm>
#include <iostream>

namespace radicc {
namespace {

std::vector<ProgramEventInfo> fetch_programs_by_station_date(
    const std::string& station_id,
    const std::string& yyyymmdd) {
  const std::string url =
      "https://radiko.jp/v3/program/station/date/" + yyyymmdd + "/" + station_id + ".xml";
  const std::string xml = fetch_programs_xml(url);
  return xml.empty() ? std::vector<ProgramEventInfo>{} : parse_programs_from_xml(xml);
}

std::vector<ProgramEventInfo> fetch_programs_from_weekly(
    const std::string& station_id,
    const std::string& yyyymmdd) {
  const std::string next_date = shift_date8(yyyymmdd, 1);
  if (next_date.empty()) return {};

  const std::string xml =
      fetch_programs_xml("https://radiko.jp/v3/program/station/weekly/" + station_id + ".xml");
  if (xml.empty()) return {};

  const std::string range_start = yyyymmdd + "050000";
  const std::string range_end = next_date + "050000";
  auto programs = parse_programs_from_xml(xml);
  programs.erase(
      std::remove_if(
          programs.begin(),
          programs.end(),
          [&](const ProgramEventInfo& program) {
            return program.ft < range_start || program.ft >= range_end;
          }),
      programs.end());
  return programs;
}

}  // namespace

std::optional<std::string> find_program_event_url(
    const std::string& station_id,
    const std::string& yyyymmdd,
    const std::string& title) {
  if (station_id.empty() || yyyymmdd.size() != 8 || title.empty()) return std::nullopt;
  const std::string xml = fetch_programs_xml("https://radiko.jp/v3/program/station/date/" + yyyymmdd + "/" + station_id + ".xml");
  if (xml.empty()) return std::nullopt;
  for (const auto& program : parse_programs_from_xml(xml)) {
    if (program.title == title) return program.event_url;
  }
  return std::nullopt;
}

std::optional<ProgramEventInfo> find_program_event_info(
    const std::string& station_id,
    const std::string& yyyymmdd,
    const std::string& title) {
  if (station_id.empty() || yyyymmdd.size() != 8 || title.empty()) return std::nullopt;
  const std::string xml = fetch_programs_xml("https://radiko.jp/v3/program/station/date/" + yyyymmdd + "/" + station_id + ".xml");
  if (xml.empty()) return std::nullopt;
  for (auto program : parse_programs_from_xml(xml)) {
    if (program.title == title) {
      fill_program_image_from_event_page(program);
      return program;
    }
  }
  return std::nullopt;
}

std::vector<ProgramEventInfo> list_programs_by_station_date(
    const std::string& station_id,
    const std::string& yyyymmdd) {
  if (station_id.empty() || yyyymmdd.size() != 8) return {};

  auto programs = fetch_programs_by_station_date(station_id, yyyymmdd);
  if (!programs.empty()) return programs;

  std::cerr << "Warning: date schedule was empty; retrying: "
            << station_id << " " << yyyymmdd << std::endl;
  programs = fetch_programs_by_station_date(station_id, yyyymmdd);
  if (!programs.empty()) return programs;

  std::cerr << "Warning: date schedule retry was empty; falling back to weekly schedule: "
            << station_id << " " << yyyymmdd << std::endl;
  return fetch_programs_from_weekly(station_id, yyyymmdd);
}

}  // namespace radicc
