// karing-style: pragma once, namespace, snake_case
#pragma once

#include "app/command_options.h"

#include <string>

namespace radicc {

void show_usage(const std::string& program_name, const std::string& command = std::string());
void parse_arguments(
    const std::string& program_name,
    const std::string& command,
    int argc,
    char* argv[],
    int start_index,
    CommandOptions& options);

} // namespace radicc
