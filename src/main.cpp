// karing-style: namespace, snake_case names, organized includes
#include "cli/arguments.h"
#include "core/radiko_auth.h"
#include "core/radiko_recorder.h"
#include "core/radiko_stream.h"
#include "core/toml_parser.h"
#include "core/url_parser.h"
#include "core/radiko_programs.h"
#include "utils/date.h"
#include "utils/env_loader.h"
#include <array>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <string>
#include <tuple>

namespace {
void print_error_and_exit(const std::string& message) {
  std::cerr << "Error: " << message << std::endl;
  std::exit(1);
}

bool has_datetime_components(const std::array<std::string, 3>& datetime) {
  return datetime[0].size() == 4 && datetime[1].size() == 4 && datetime[2].size() == 6;
}

bool has_path_separator(const std::string& value) {
  return value.find('/') != std::string::npos || value.find('\\') != std::string::npos;
}

bool is_directory_output_override(const std::string& value) {
  if (value.empty()) return false;
  if (value == "." || value == ".." || value == "./" || value == "../") return true;
  return value.back() == '/' || value.back() == '\\';
}

std::string ensure_trailing_separator(const std::filesystem::path& path) {
  std::string value = path.empty() ? std::string("./") : path.string();
  if (value.empty()) value = "./";
  if (value.back() != '/') value += '/';
  return value;
}

std::string expand_output_dir(std::string value) {
  value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
  const char* home = std::getenv("HOME");
  if (home) {
    size_t pos = value.find("$HOME");
    if (pos != std::string::npos) value.replace(pos, 5, home);
    if (!value.empty() && value.front() == '~') value.replace(0, 1, home);
  }
  return ensure_trailing_separator(std::filesystem::path(value));
}

std::string build_output_path(const std::string& output_dir, const std::string& dir_name, const std::string& filename) {
  return dir_name.empty() ? (output_dir + filename) : (output_dir + dir_name + "/" + filename);
}

std::time_t to_time_utc_like(const std::string& yyyymmddhhmmss) {
  if (yyyymmddhhmmss.size() != 14) return 0;
  std::tm t{}; t.tm_isdst = -1;
  int Y = std::stoi(yyyymmddhhmmss.substr(0,4));
  int m = std::stoi(yyyymmddhhmmss.substr(4,2));
  int d = std::stoi(yyyymmddhhmmss.substr(6,2));
  int H = std::stoi(yyyymmddhhmmss.substr(8,2));
  int M = std::stoi(yyyymmddhhmmss.substr(10,2));
  int S = std::stoi(yyyymmddhhmmss.substr(12,2));
  t.tm_year = Y - 1900; t.tm_mon = m - 1; t.tm_mday = d;
  t.tm_hour = H; t.tm_min = M; t.tm_sec = S;
  return std::mktime(&t);
}

int diff_minutes(const std::string& ft, const std::string& to) {
  std::time_t t_ft = to_time_utc_like(ft);
  std::time_t t_to = to_time_utc_like(to);
  if (t_ft == 0 || t_to == 0) return 0;
  double seconds = std::difftime(t_to, t_ft);
  if (seconds <= 0) return 0;
  return static_cast<int>(seconds / 60);
}

void validate_variables(const std::string& station_id, const std::string& start_time,
                        const std::string& end_time, const std::string& filename, int duration) {
  if (station_id.empty()) print_error_and_exit("Station ID is required.");
  if (start_time.empty()) print_error_and_exit("Start time is required.");
  if (end_time.empty()) print_error_and_exit("End time is required.");
  if (duration == 0) print_error_and_exit("Duration must be greater than 0.");
}

std::string json_escape(const std::string& input) {
  static constexpr char hex[] = "0123456789ABCDEF";
  std::string escaped;
  escaped.reserve(input.size() + 8);
  for (unsigned char c : input) {
    switch (c) {
      case '\\': escaped += "\\\\"; break;
      case '"': escaped += "\\\""; break;
      case '\b': escaped += "\\b"; break;
      case '\f': escaped += "\\f"; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default:
        if (c < 0x20) {
          escaped += "\\u00";
          escaped.push_back(hex[c >> 4]);
          escaped.push_back(hex[c & 0x0F]);
        } else {
          escaped.push_back(static_cast<char>(c));
        }
    }
  }
  return escaped;
}
} // namespace

int main(int argc, char* argv[]) {
  using namespace radicc;
  std::string target, id, url, station_id, output_option, pfm, weekday, dir, title;
  std::string image_url;       // final image
  std::string img_toml;        // fallback image from TOML (used only if no fetched image)
  std::string pfm_toml;        // fallback pfm from TOML (used only if no fetched pfm)
  bool fetch_only = false;
  bool json_output = false;
  bool cli_date_offset_set = false;
  int duration = 0;
  std::array<std::string, 3> datetime = {"", "", ""};
  int date_offset = 0; // days to subtract for filename date
  std::string toml_base_dir;

  parse_arguments(argc, argv, target, id, url, duration, date_offset, cli_date_offset_set, output_option, weekday, pfm, fetch_only, json_output);

  const auto global_config = parse_toml_global();
  if (global_config.count("base_dir")) toml_base_dir = global_config.at("base_dir");

  bool has_valid_config = false;

  // URL mode: parse station + ft from URL and resolve directly. Ignore TOML entirely.
  if (!url.empty() && !has_valid_config) {
    auto parsed = parse_radiko_url(url);
    if (!parsed) {
      print_error_and_exit("Invalid URL format.");
    }
    std::array<std::string,3> dt; std::tie(station_id, dt) = *parsed;
    const std::string ft = dt[0] + dt[1] + dt[2];
    // Validate window: not future, within last 7 days
    auto to_time = [](const std::string& yyyymmddhhmmss) -> std::time_t {
      std::tm t{}; t.tm_isdst = -1;
      std::istringstream ss(yyyymmddhhmmss);
      int Y = std::stoi(yyyymmddhhmmss.substr(0,4));
      int m = std::stoi(yyyymmddhhmmss.substr(4,2));
      int d = std::stoi(yyyymmddhhmmss.substr(6,2));
      int H = std::stoi(yyyymmddhhmmss.substr(8,2));
      int M = std::stoi(yyyymmddhhmmss.substr(10,2));
      int S = std::stoi(yyyymmddhhmmss.substr(12,2));
      t.tm_year = Y - 1900; t.tm_mon = m - 1; t.tm_mday = d;
      t.tm_hour = H; t.tm_min = M; t.tm_sec = S;
      return std::mktime(&t);
    };
    std::time_t now = std::time(nullptr);
    std::time_t t_ft = to_time(ft);
    if (t_ft > now) print_error_and_exit("URL points to a future program.");
    if (now - t_ft > 7 * 24 * 3600) print_error_and_exit("Program is older than 7 days.");

    auto info = find_program_by_station_ft(station_id, ft);
    if (!info) print_error_and_exit("Program not found by station+ft in weekly XML.");

    // Apply fetched info
    title = info->title.empty() ? title : info->title;
    pfm = info->pfm;
    image_url = info->image_url;
    if (info->ft.size() == 14 && info->to.size() == 14) {
      datetime = { info->ft.substr(0,4), info->ft.substr(4,4), info->ft.substr(8) };
      int dmins = diff_minutes(info->ft, info->to);
      if (dmins > 0) duration = dmins;
    }
    has_valid_config = true;
  }
  if (url.empty() && !target.empty()) {
    auto config = parse_toml(target);
    if (!config.empty()) {
      has_valid_config = true;
      station_id = config["station"];
      // optional fields only
      if (id.empty() && config.count("id")) id = config["id"];
      if (config.count("title")) title = config["title"];
      if (config.count("img")) img_toml = config["img"];
      if (config.count("pfm")) pfm_toml = config["pfm"];
      if (!cli_date_offset_set && config.count("date_offset")) { try { date_offset = std::stoi(config["date_offset"]); } catch (...) {} }
      if (config.count("dir")) dir = config["dir"];
      if (output_option.empty() && config.count("filename")) output_option = config["filename"];
    } else {
      std::cerr << "Warning: Section not found by -t. Will try -i (if provided).\n";
    }
  }

  if (url.empty() && !has_valid_config && !id.empty()) {
    auto config = parse_toml_by_id(id);
    if (!config.empty()) {
      has_valid_config = true;
      station_id = config["station"];
      // optional fields only
      if (config.count("title")) title = config["title"];
      if (config.count("img")) img_toml = config["img"];
      if (config.count("pfm")) pfm_toml = config["pfm"];
      if (!cli_date_offset_set && config.count("date_offset")) { try { date_offset = std::stoi(config["date_offset"]); } catch (...) {} }
      if (config.count("dir")) dir = config["dir"];
      if (output_option.empty() && config.count("filename")) output_option = config["filename"];
      if (id.empty() && config.count("id")) id = config["id"];
    } else {
      std::cerr << "Warning: Could not find section by -i id.\n";
    }
  }

  // If title is provided, resolve by weekly XML (nearest past entry by exact title).
  // This is only for TOML mode (URL mode already resolved by station+ft).
  if (url.empty()) {
    if (title.empty()) title = !target.empty() ? target : id;
    if (!station_id.empty() && !title.empty()) {
      auto info = find_nearest_weekly_program_info(station_id, title);
      if (info) {
      // fetched values; TOML priority applies only to dir/filename (save-time fields)
        if (!info->image_url.empty()) image_url = info->image_url;
        // Prefer fetched time/duration/personality
        if (info->ft.size() == 14 && info->to.size() == 14) {
          datetime = { info->ft.substr(0,4), info->ft.substr(4,4), info->ft.substr(8) };
          int dmins = diff_minutes(info->ft, info->to);
          if (dmins > 0) duration = dmins;
        }
        if (pfm.empty() && !info->pfm.empty()) pfm = info->pfm;
      }
    }
  }
  // If URL mode, do not apply any TOML fallback (single-shot recording).
  if (url.empty()) {
    // If no fetched image, apply TOML fallback img
    if (image_url.empty() && !img_toml.empty()) image_url = img_toml;
    // If no fetched pfm, apply TOML fallback pfm
    if (pfm.empty() && !pfm_toml.empty()) pfm = pfm_toml;
  }

  // url is retained for future use; not used for config selection or recording yet.

  if (!has_valid_config) print_error_and_exit("Missing required configuration. Provide either -t <section> or -i <id>.");

  if (!has_datetime_components(datetime)) {
    print_error_and_exit("Failed to resolve broadcast datetime. Check network connectivity or schedule data.");
  }

  std::string start_time = generate_14digit_datetime(datetime, 0);
  std::string end_time = generate_14digit_datetime(datetime, duration);
  const bool explicit_output_path = has_path_separator(output_option) || is_directory_output_override(output_option);
  // Default dir only in TOML mode: create <title>/...
  if (url.empty() && dir.empty() && !title.empty() && !explicit_output_path) dir = title;
  // Build filename: base (TOML filename or title) + -YYYYMMDD + .m4a (always append)
  std::string filename;
  {
    auto ymd = datetime[0] + datetime[1];
    if (ymd.size() == 8 && date_offset != 0) {
      std::tm t{}; std::istringstream ss(ymd); ss >> std::get_time(&t, "%Y%m%d");
      t.tm_mday -= date_offset; std::mktime(&t);
      std::ostringstream os; os << std::put_time(&t, "%Y%m%d"); ymd = os.str();
    }
    std::string base = output_option.empty() || explicit_output_path ? title : output_option;
    filename = base + "-" + ymd + ".m4a";
  }

  validate_variables(station_id, start_time, end_time, filename, duration);

  if (json_output) {
#if defined(_WIN32)
    _putenv_s("RADICC_SUPPRESS_ENV_LOG", "1");
#else
    setenv("RADICC_SUPPRESS_ENV_LOG", "1", 1);
#endif
  }

  load_env_from_file();
  std::string radiko_user, radiko_pass, output_dir;
  if (!check_radiko_credentials(radiko_user, radiko_pass, output_dir)) {
    if (!json_output) {
      std::cerr << "No Radiko credentials found. Proceeding without login." << std::endl;
    }
  }

  // filename already constructed; no placeholder replacement needed
  std::string session_id;
  bool is_areafree = false;
  if (!radiko_user.empty() && !radiko_pass.empty()) {
    auto login = login_to_radiko(radiko_user, radiko_pass);
    if (login) {
      session_id = login->session_id;
      is_areafree = login->is_areafree;
    } else if (!json_output) {
      std::cerr << "Warning: Login failed, proceeding without Radiko Premium access." << std::endl;
    }
  } else {
    if (!json_output) {
      std::cerr << "No Radiko credentials provied, proceeding without Radiko Premium access." << std::endl;
    }
  }

  std::string resolved_output_dir = output_dir;
  std::string resolved_dir = dir;
  std::string resolved_filename = filename;
  if (!output_option.empty() && explicit_output_path) {
    const std::filesystem::path override_path(output_option);
    if (is_directory_output_override(output_option)) {
      resolved_output_dir = ensure_trailing_separator(override_path);
      resolved_dir.clear();
    } else {
      resolved_output_dir = ensure_trailing_separator(
          override_path.has_parent_path() ? override_path.parent_path() : std::filesystem::path("."));
      resolved_dir.clear();
      resolved_filename = override_path.filename().string();
    }
  } else if (!toml_base_dir.empty()) {
    resolved_output_dir = expand_output_dir(toml_base_dir);
  }

  const std::string output_path = build_output_path(resolved_output_dir, resolved_dir, resolved_filename);
  const std::filesystem::path absolute_output_path = std::filesystem::absolute(output_path);

  if (!fetch_only) {
    auto auth_state = authorize_radiko(session_id);
    if (!auth_state) print_error_and_exit("Authorization failed.");
    auto stream_plan = build_timefree_stream_plan(station_id, start_time, end_time, is_areafree, *auth_state);
    if (!stream_plan) print_error_and_exit("Failed to resolve timefree stream request.");
    if (!record_radiko(*stream_plan, resolved_filename, pfm, title, resolved_dir, resolved_output_dir, image_url)) {
      print_error_and_exit("Failed to record the broadcast.");
    }
    logout_from_radiko(session_id);
  } else {
    std::string notice = "--fetch was specified, recording was skipped.";
    if (!json_output) {
      std::cout << notice << std::endl;
    }
  }

  const std::string resolved_id = id.empty() ? target : id;
  const std::string absolute_path_str = absolute_output_path.string();
  const std::string directory_path = resolved_dir.empty()
      ? resolved_output_dir
      : resolved_output_dir + resolved_dir + "/";

  const std::string start_date = start_time.size() >= 8 ? start_time.substr(0, 8) : std::string();

  if (json_output) {
    std::ostringstream json;
    json << '{'
         << "\"id\":\"" << json_escape(resolved_id) << "\",";
    json << "\"station_id\":\"" << json_escape(station_id) << "\",";
    json << "\"start_time\":\"" << json_escape(start_time) << "\",";
    json << "\"end_time\":\"" << json_escape(end_time) << "\",";
    json << "\"duration_minutes\":" << duration << ',';
    json << "\"output_file\":\"" << json_escape(resolved_filename) << "\",";
    json << "\"filepath\":\"" << json_escape(absolute_path_str) << "\",";
    json << "\"title\":\"" << json_escape(title) << "\",";
    json << "\"dir\":\"" << json_escape(resolved_dir) << "\",";
    json << "\"directory\":\"" << json_escape(directory_path) << "\",";
    json << "\"pfm\":\"" << json_escape(pfm) << "\",";
    json << "\"image_url\":\"" << json_escape(image_url) << "\",";
    json << "\"date\":\"" << json_escape(start_date) << "\",";
    json << "\"fetch_only\":" << (fetch_only ? "true" : "false")
         << '}';
    std::cout << json.str() << std::endl;
  } else {
    std::cout << "\n";
    std::cout << "ID: " << resolved_id << "\n";
    std::cout << "Station: " << station_id << "\n";
    std::cout << "Time: " << start_time << " - " << end_time << "\n";
    std::cout << "Duration: " << duration << " minutes\n";
    std::cout << "Output File: " << resolved_filename << "\n";
    std::cout << "pfm: " << pfm << "\n";
    if (!resolved_dir.empty()) {
      std::cout << "dir: " << resolved_dir << "\n";
    }
    std::cout << "Directory: " << directory_path << "\n\n";
    if (!image_url.empty()) std::cout << "(image url)   " << image_url << "\n";
    if (fetch_only) {
      std::cout << "Fetch completed successfully.\n";
    } else {
      std::cout << "Recording completed successfully.\n";
    }
  }
  return 0;
}
