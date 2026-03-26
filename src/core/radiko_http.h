#pragma once

#include <optional>
#include <string>
#include <vector>

namespace radicc {

int run_command_capture(const std::vector<std::string>& args, std::string& output);
std::optional<std::string> curl_text(const std::vector<std::string>& args);
std::optional<std::string> curl_get_text(const std::string& url);
std::string trim_crlf(std::string value);

}  // namespace radicc
