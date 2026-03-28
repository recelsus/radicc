#include "app/record_command.h"

#include "app/common.h"
#include "app/output_path.h"
#include "app/record_resolver.h"
#include "core/radiko_auth.h"
#include "core/radiko_recorder.h"
#include "core/radiko_stream.h"
#include "utils/date.h"
#include "utils/env_loader.h"

#include <iostream>
#include <sstream>

namespace radicc {

int run_record_command(const CommandOptions& options) {
  if (options.json_output) {
#if defined(_WIN32)
    _putenv_s("RADICC_SUPPRESS_ENV_LOG", "1");
#else
    setenv("RADICC_SUPPRESS_ENV_LOG", "1", 1);
#endif
  }

  load_env_from_file();
  std::string radiko_user, radiko_pass, output_dir;
  if (!check_radiko_credentials(radiko_user, radiko_pass, output_dir) && !options.json_output) {
    std::cerr << "No Radiko credentials found. Proceeding without login." << std::endl;
  }

  std::string session_id;
  bool is_areafree = false;
  if (!radiko_user.empty() && !radiko_pass.empty()) {
    auto login = login_to_radiko(radiko_user, radiko_pass);
    if (login) {
      session_id = login->session_id;
      is_areafree = login->is_areafree;
    } else if (!options.json_output) {
      std::cerr << "Warning: Login failed, proceeding without Radiko Premium access." << std::endl;
    }
  } else if (!options.json_output) {
    std::cerr << "No Radiko credentials provied, proceeding without Radiko Premium access." << std::endl;
  }

  const int max_timefree_days = 30;
  ResolvedRecord resolved = resolve_record_command(options, max_timefree_days);
  const OutputPaths paths = resolve_output_paths(
      output_dir, resolved.toml_base_dir, options.output, resolved.title, resolved.dir_name,
      resolved.datetime, resolved.date_offset);
  const std::string start_time = generate_14digit_datetime(resolved.datetime, 0);
  const std::string end_time = generate_14digit_datetime(resolved.datetime, resolved.duration);

  if (!resolved.fetch_only) {
    auto auth_state = authorize_radiko(session_id);
    if (!auth_state) print_error_and_exit("Authorization failed.");
    auto stream_plan = build_timefree_stream_plan(resolved.station_id, start_time, end_time, is_areafree, *auth_state);
    if (!stream_plan) print_error_and_exit("Failed to resolve timefree stream request.");
    if (!record_radiko(*stream_plan, paths.filename, resolved.pfm, resolved.title, paths.dir_name, paths.output_dir, resolved.image_url)) {
      print_error_and_exit("Failed to record the broadcast.");
    }
    logout_from_radiko(session_id);
  } else if (!resolved.json_output) {
    std::cout << "--fetch was specified, recording was skipped." << std::endl;
  }

  if (resolved.json_output) {
    std::ostringstream json;
    json << '{'
         << "\"id\":\"" << json_escape(options.id.empty() ? options.target : options.id) << "\","
         << "\"station_id\":\"" << json_escape(resolved.station_id) << "\","
         << "\"start_time\":\"" << json_escape(start_time) << "\","
         << "\"end_time\":\"" << json_escape(end_time) << "\","
         << "\"duration_minutes\":" << resolved.duration << ','
         << "\"output_file\":\"" << json_escape(paths.filename) << "\","
         << "\"filepath\":\"" << json_escape(paths.absolute_path) << "\","
         << "\"title\":\"" << json_escape(resolved.title) << "\","
         << "\"dir\":\"" << json_escape(paths.dir_name) << "\","
         << "\"directory\":\"" << json_escape(paths.directory_path) << "\","
         << "\"pfm\":\"" << json_escape(resolved.pfm) << "\","
         << "\"image_url\":\"" << json_escape(resolved.image_url) << "\","
         << "\"date\":\"" << json_escape(start_time.substr(0, 8)) << "\","
         << "\"fetch_only\":" << (resolved.fetch_only ? "true" : "false")
         << '}';
    std::cout << json.str() << std::endl;
    return 0;
  }

  std::cout << "\n";
  std::cout << "ID: " << (options.id.empty() ? options.target : options.id) << "\n";
  std::cout << "Station: " << resolved.station_id << "\n";
  std::cout << "Time: " << start_time << " - " << end_time << "\n";
  std::cout << "Duration: " << resolved.duration << " minutes\n";
  std::cout << "Output File: " << paths.filename << "\n";
  std::cout << "pfm: " << resolved.pfm << "\n";
  if (!paths.dir_name.empty()) std::cout << "dir: " << paths.dir_name << "\n";
  std::cout << "Directory: " << paths.directory_path << "\n\n";
  if (!resolved.image_url.empty()) std::cout << "(image url)   " << resolved.image_url << "\n";
  std::cout << (resolved.fetch_only ? "Fetch completed successfully.\n" : "Recording completed successfully.\n");
  return 0;
}

}  // namespace radicc
