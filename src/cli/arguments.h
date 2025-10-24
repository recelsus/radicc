// karing-style: pragma once, namespace, snake_case
#pragma once

#include <string>

namespace radicc {

void parse_arguments(
    int argc,
    char* argv[],
    std::string& target,
    std::string& id,
    std::string& url,
    int& duration,
    std::string& output,
    std::string& weekday,
    std::string& personality,
    bool& dryrun,
    bool& json_output);

} // namespace radicc
