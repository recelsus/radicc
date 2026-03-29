#pragma once

#include "app/command_options.h"
#include "app/output_path.h"
#include "app/record_resolver.h"

#include <string>

namespace radicc {

struct RecordExecutionResult {
  ResolvedRecord resolved;
  OutputPaths paths;
  std::string start_time;
  std::string end_time;
};

RecordExecutionResult execute_record_request(const CommandOptions& options);
std::string build_record_result_json(const CommandOptions& options, const RecordExecutionResult& result);

}  // namespace radicc
