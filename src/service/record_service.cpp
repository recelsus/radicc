#include "service/record_service.h"

#include "app/common.h"
#include "core/radiko_auth.h"
#include "core/radiko_recorder.h"
#include "core/radiko_stream.h"
#include "utils/date.h"
#include "utils/env_loader.h"

#include <iostream>
#include <sstream>

namespace radicc {

RecordExecutionResult execute_record_request(const CommandOptions& options) {
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

  RecordExecutionResult result;
  result.resolved = resolve_record_command(options, 30);
  result.paths = resolve_output_paths(
      output_dir, result.resolved.toml_base_dir, options.output, result.resolved.title, result.resolved.dir_name,
      result.resolved.datetime, result.resolved.date_offset);
  result.start_time = generate_14digit_datetime(result.resolved.datetime, 0);
  result.end_time = generate_14digit_datetime(result.resolved.datetime, result.resolved.duration);

  if (!result.resolved.fetch_only) {
    auto auth_state = authorize_radiko(session_id);
    if (!auth_state) print_error_and_exit("Authorization failed.");
    auto stream_plan = build_timefree_stream_plan(
        result.resolved.station_id, result.start_time, result.end_time, is_areafree, *auth_state);
    if (!stream_plan) print_error_and_exit("Failed to resolve timefree stream request.");
    if (!record_radiko(
            *stream_plan, result.paths.filename, result.resolved.pfm, result.resolved.title,
            result.paths.dir_name, result.paths.output_dir, result.resolved.image_url)) {
      print_error_and_exit("Failed to record the broadcast.");
    }
    logout_from_radiko(session_id);
  } else if (!options.json_output) {
    std::cout << "--fetch was specified, recording was skipped." << std::endl;
  }

  return result;
}

std::string build_record_result_json(const CommandOptions& options, const RecordExecutionResult& result) {
  std::ostringstream json;
  json << '{'
       << "\"id\":\"" << json_escape(options.id.empty() ? options.target : options.id) << "\","
       << "\"station_id\":\"" << json_escape(result.resolved.station_id) << "\","
       << "\"start_time\":\"" << json_escape(result.start_time) << "\","
       << "\"end_time\":\"" << json_escape(result.end_time) << "\","
       << "\"duration_minutes\":" << result.resolved.duration << ','
       << "\"output_file\":\"" << json_escape(result.paths.filename) << "\","
       << "\"filepath\":\"" << json_escape(result.paths.absolute_path) << "\","
       << "\"title\":\"" << json_escape(result.resolved.title) << "\","
       << "\"dir\":\"" << json_escape(result.paths.dir_name) << "\","
       << "\"directory\":\"" << json_escape(result.paths.directory_path) << "\","
       << "\"pfm\":\"" << json_escape(result.resolved.pfm) << "\","
       << "\"image_url\":\"" << json_escape(result.resolved.image_url) << "\","
       << "\"date\":\"" << json_escape(result.start_time.substr(0, 8)) << "\","
       << "\"fetch_only\":" << (result.resolved.fetch_only ? "true" : "false")
       << '}';
  return json.str();
}

}  // namespace radicc
