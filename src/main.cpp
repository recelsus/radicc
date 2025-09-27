// karing-style: namespace, snake_case names, organized includes
#include "cli/arguments.h"
#include "core/radiko_recorder.h"
#include "core/toml_parser.h"
#include "core/url_parser.h"
#include "core/radiko_programs.h"
#include "utils/date.h"
#include "utils/env_loader.h"
#include <array>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>

namespace {
void print_error_and_exit(const std::string& message) {
  std::cerr << "Error: " << message << std::endl;
  std::exit(1);
}

void validate_variables(const std::string& station_id, const std::string& start_time,
                        const std::string& end_time, const std::string& filename, int duration) {
  if (station_id.empty()) print_error_and_exit("Station ID is required.");
  if (start_time.empty()) print_error_and_exit("Start time is required.");
  if (end_time.empty()) print_error_and_exit("End time is required.");
  if (duration == 0) print_error_and_exit("Duration must be greater than 0.");
}
} // namespace

int main(int argc, char* argv[]) {
  using namespace radicc;
  std::string target, id, url, station_id, filename, pfm, weekday, dir, title;
  std::string image_url;       // final image
  std::string img_toml;        // fallback image from TOML (used only if no fetched image)
  std::string pfm_toml;        // fallback pfm from TOML (used only if no fetched pfm)
  bool dryrun = false;
  int duration = 0;
  std::array<std::string, 3> datetime = {"", "", ""};
  int date_offset = 0; // days to subtract for filename date

  parse_arguments(argc, argv, target, id, url, duration, filename, weekday, pfm, dryrun);

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
      int sh = std::stoi(info->ft.substr(8,2));
      int sm = std::stoi(info->ft.substr(10,2));
      int eh = std::stoi(info->to.substr(8,2));
      int em = std::stoi(info->to.substr(10,2));
      int dmins = (eh*60 + em) - (sh*60 + sm);
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
      if (config.count("date_offset")) { try { date_offset = std::stoi(config["date_offset"]); } catch (...) {} }
      if (config.count("dir")) dir = config["dir"];
      if (config.count("filename")) filename = config["filename"];
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
      if (config.count("date_offset")) { try { date_offset = std::stoi(config["date_offset"]); } catch (...) {} }
      if (config.count("dir")) dir = config["dir"];
      if (config.count("filename")) filename = config["filename"];
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
          int sh = std::stoi(info->ft.substr(8,2));
          int sm = std::stoi(info->ft.substr(10,2));
          int eh = std::stoi(info->to.substr(8,2));
          int em = std::stoi(info->to.substr(10,2));
          int d = (eh*60 + em) - (sh*60 + sm);
          if (d > 0) duration = d;
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

  std::string start_time = generate_14digit_datetime(datetime, 0);
  std::string end_time = generate_14digit_datetime(datetime, duration);
  // Default dir only in TOML mode: create <title>/...
  if (url.empty() && dir.empty() && !title.empty()) dir = title;
  // Build filename: base (TOML filename or title) + -YYYYMMDD + .m4a (always append)
  {
    auto ymd = datetime[0] + datetime[1];
    if (ymd.size() == 8 && date_offset != 0) {
      std::tm t{}; std::istringstream ss(ymd); ss >> std::get_time(&t, "%Y%m%d");
      t.tm_mday -= date_offset; std::mktime(&t);
      std::ostringstream os; os << std::put_time(&t, "%Y%m%d"); ymd = os.str();
    }
    std::string base = filename.empty() ? title : filename;
    filename = base + "-" + ymd + ".m4a";
  }

  validate_variables(station_id, start_time, end_time, filename, duration);

  load_env_from_file();
  std::string radiko_user, radiko_pass, output_dir;
  if (!check_radiko_credentials(radiko_user, radiko_pass, output_dir)) {
    std::cerr << "No Radiko credentials found. Proceeding without login." << std::endl;
  }

  // filename already constructed; no placeholder replacement needed
  std::string authtoken, session_id;
  bool logged_in = false;
  if (!radiko_user.empty() && !radiko_pass.empty()) {
    logged_in = login_to_radiko(radiko_user, radiko_pass, session_id);
    if (!logged_in) std::cerr << "Warning: Login failed, proceeding without Radiko Premium access." << std::endl;
  } else {
    std::cerr << "No Radiko credentials provied, proceeding without Radiko Premium access." << std::endl;
  }

  if (!dryrun) {
    if (!authorize_radiko(authtoken, session_id)) print_error_and_exit("Authorization failed.");
    if (!record_radiko(station_id, start_time, end_time, filename, authtoken, pfm, title, dir, output_dir, image_url)) {
      print_error_and_exit("Failed to record the broadcast.");
    }
    std::string session;
    logout_from_radiko(session);
  } else {
    std::cout << "--dry-run was specified, not execute." << std::endl;
  }

  std::cout << "\n";
  std::cout << "ID: " << (id.empty() ? target : id) << "\n";
  std::cout << "Station: " << station_id << "\n";
  std::cout << "Time: " << start_time << " - " << end_time << "\n";
  std::cout << "Duration: " << duration << " minutes\n";
  std::cout << "Output File: " << filename << "\n";
  std::cout << "pfm: " << pfm << "\n";
  std::cout << "dir: " << dir << "\n\n";
  if (!image_url.empty()) std::cout << "(image url)   " << image_url << "\n";
  std::cout << "Recording completed successfully.\n";
  return 0;
}
