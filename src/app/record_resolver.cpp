#include "app/record_resolver.h"

#include "app/common.h"
#include "core/radiko_programs.h"
#include "core/toml_parser.h"
#include "core/url_parser.h"

#include <tuple>

namespace radicc {
namespace {

void validate_url_age(const std::string& ft, int max_timefree_days) {
  const std::time_t now = std::time(nullptr);
  const std::time_t t_ft = to_time_utc_like(ft);
  if (t_ft > now) print_error_and_exit("URL points to a future program.");
  if (now - t_ft > static_cast<std::time_t>(max_timefree_days) * 24 * 3600) {
    print_error_and_exit("Program is older than " + std::to_string(max_timefree_days) + " days.");
  }
}

void apply_url_mode(const CommandOptions& options, ResolvedRecord& resolved, int max_timefree_days) {
  auto parsed = parse_radiko_url(options.url);
  if (!parsed) print_error_and_exit("Invalid URL format.");

  std::array<std::string, 3> dt;
  std::tie(resolved.station_id, dt) = *parsed;
  const std::string ft = dt[0] + dt[1] + dt[2];
  validate_url_age(ft, max_timefree_days);

  auto info = find_program_by_station_ft(resolved.station_id, ft);
  if (!info) print_error_and_exit("Program not found by station+ft.");
  resolved.title = info->title;
  resolved.pfm = info->pfm;
  resolved.image_url = info->image_url;
  if (info->ft.size() == 14 && info->to.size() == 14) {
    resolved.datetime = {info->ft.substr(0, 4), info->ft.substr(4, 4), info->ft.substr(8)};
    resolved.duration = diff_minutes(info->ft, info->to);
  }
}

void apply_toml_config(const CommandOptions& options, ResolvedRecord& resolved) {
  auto config = !options.target.empty() ? parse_toml(options.target) : parse_toml_by_id(options.id);
  if (config.empty() && !options.target.empty() && !options.id.empty()) config = parse_toml_by_id(options.id);
  if (config.empty()) print_error_and_exit("Missing required configuration. Provide either -t <section> or -i <id>.");

  resolved.station_id = config["station"];
  resolved.title = config.count("title") ? config["title"] : (!options.target.empty() ? options.target : options.id);
  resolved.dir_name = config.count("dir") ? config["dir"] : std::string();
  resolved.pfm = config.count("pfm") ? config["pfm"] : std::string();
  resolved.image_url = config.count("img") ? config["img"] : std::string();
  if (!options.date_offset_set && config.count("date_offset")) resolved.date_offset = std::stoi(config["date_offset"]);

  auto info = find_nearest_weekly_program_info(resolved.station_id, resolved.title);
  if (info) {
    if (!info->image_url.empty()) resolved.image_url = info->image_url;
    if (!info->pfm.empty()) resolved.pfm = info->pfm;
    if (info->ft.size() == 14 && info->to.size() == 14) {
      resolved.datetime = {info->ft.substr(0, 4), info->ft.substr(4, 4), info->ft.substr(8)};
      resolved.duration = diff_minutes(info->ft, info->to);
    }
  }
}

}  // namespace

ResolvedRecord resolve_record_command(const CommandOptions& options, int max_timefree_days) {
  ResolvedRecord resolved;
  resolved.json_output = options.json_output;
  resolved.fetch_only = options.fetch_only;
  resolved.duration = options.duration;
  resolved.date_offset = options.date_offset;

  const auto global_config = parse_toml_global();
  if (global_config.count("base_dir")) resolved.toml_base_dir = global_config.at("base_dir");

  if (!options.url.empty()) {
    apply_url_mode(options, resolved, max_timefree_days);
  } else {
    apply_toml_config(options, resolved);
  }

  if (!has_datetime_components(resolved.datetime)) {
    print_error_and_exit("Failed to resolve broadcast datetime. Check network connectivity or schedule data.");
  }
  if (resolved.duration <= 0) print_error_and_exit("Duration must be greater than 0.");
  if (resolved.station_id.empty()) print_error_and_exit("Station ID is required.");
  return resolved;
}

}  // namespace radicc
