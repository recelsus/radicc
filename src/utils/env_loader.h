#pragma once
#include <string>

namespace radicc {

bool file_exists(const std::string& path);
std::string get_config_path_for_env(const std::string& filename);
void load_env_file(const std::string& filepath);
void load_env_from_file();
bool check_radiko_credentials(std::string& radikoUser, std::string& radikoPass, std::string& outputDir);

} // namespace radicc

