#include "core/radiko_programs.h"

#include "core/radiko_programs_xml.h"

namespace radicc {

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
  const std::string xml = fetch_programs_xml("https://radiko.jp/v3/program/station/date/" + yyyymmdd + "/" + station_id + ".xml");
  return xml.empty() ? std::vector<ProgramEventInfo>{} : parse_programs_from_xml(xml);
}

}  // namespace radicc
