#include "app/list_command.h"

#include "app/common.h"
#include "core/radiko_programs.h"
#include "utils/date.h"

#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

namespace radicc {
namespace {

std::pair<std::string, std::vector<ProgramEventInfo>> resolve_programs_for_list(
    const CommandOptions& options) {
  if (!options.date.empty()) {
    if (!is_valid_date8(options.date)) print_error_and_exit("Date must be in YYYYMMDD format.");
    return {options.date, list_programs_by_station_date(options.station_id, options.date)};
  }

  const std::string today = current_yyyymmdd_jst();
  const std::vector<std::string> candidates = {
      today,
      shift_date8(today, -1),
      shift_date8(today, 1),
  };

  for (const auto& candidate : candidates) {
    if (candidate.empty()) continue;
    auto programs = list_programs_by_station_date(options.station_id, candidate);
    if (!programs.empty()) return {candidate, std::move(programs)};
  }

  return {today, {}};
}

}  // namespace

int run_list_command(const CommandOptions& options) {
  if (options.station_id.empty()) print_error_and_exit("--station-id is required for list.");

  auto [schedule_date, programs] = resolve_programs_for_list(options);
  if (programs.empty()) print_error_and_exit("No programs found for station/date.");

  if (options.json_output) {
    std::ostringstream json;
    json << '[';
    for (size_t i = 0; i < programs.size(); ++i) {
      const auto& p = programs[i];
      if (i > 0) json << ',';
      json << '{'
           << "\"station_id\":\"" << json_escape(options.station_id) << "\","
           << "\"date\":\"" << json_escape(schedule_date) << "\","
           << "\"title\":\"" << json_escape(p.title) << "\","
           << "\"ft\":\"" << json_escape(p.ft) << "\","
           << "\"to\":\"" << json_escape(p.to) << "\","
           << "\"pfm\":\"" << json_escape(p.pfm) << "\","
           << "\"event_url\":\"" << json_escape(p.event_url) << "\","
           << "\"url\":\"" << json_escape(build_timefree_url(options.station_id, p.ft)) << "\","
           << "\"image_url\":\"" << json_escape(p.image_url) << "\","
           << "\"img\":\"" << json_escape(p.img) << "\""
           << '}';
    }
    json << ']';
    std::cout << json.str() << std::endl;
    return 0;
  }

  std::cout << "Station: " << options.station_id << "\n";
  std::cout << "Date: " << schedule_date << "\n\n";
  for (const auto& p : programs) {
    std::cout << time_hhmm(p.ft) << "-" << time_hhmm(p.to) << " | " << p.title;
    if (!p.pfm.empty()) std::cout << " | " << p.pfm;
    const auto url = build_timefree_url(options.station_id, p.ft);
    if (!url.empty()) std::cout << " | " << url;
    std::cout << "\n";
  }
  return 0;
}

}  // namespace radicc
