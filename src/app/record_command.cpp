#include "app/record_command.h"

#include "service/record_service.h"

#include <iostream>

namespace radicc {

int run_record_command(const CommandOptions& options) {
  const auto result = execute_record_request(options);

  if (options.json_output) {
    std::cout << build_record_result_json(options, result) << std::endl;
    return 0;
  }

  std::cout << "\n";
  std::cout << "ID: " << (options.id.empty() ? options.target : options.id) << "\n";
  std::cout << "Station: " << result.resolved.station_id << "\n";
  std::cout << "Time: " << result.start_time << " - " << result.end_time << "\n";
  std::cout << "Duration: " << result.resolved.duration << " minutes\n";
  std::cout << "Output File: " << result.paths.filename << "\n";
  std::cout << "pfm: " << result.resolved.pfm << "\n";
  if (!result.paths.dir_name.empty()) std::cout << "dir: " << result.paths.dir_name << "\n";
  std::cout << "Directory: " << result.paths.directory_path << "\n\n";
  if (!result.resolved.image_url.empty()) std::cout << "(image url)   " << result.resolved.image_url << "\n";
  std::cout << (result.resolved.fetch_only ? "Fetch completed successfully.\n" : "Recording completed successfully.\n");
  return 0;
}

}  // namespace radicc
