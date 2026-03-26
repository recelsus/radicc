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
    int& date_offset,
    bool& date_offset_set,
    std::string& output,
    std::string& weekday,
    std::string& personality,
    bool& fetch_only,
    bool& json_output);

} // namespace radicc
