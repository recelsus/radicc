#pragma once

#include <array>
#include <string>

namespace radicc {

struct OutputPaths {
  std::string output_dir;
  std::string dir_name;
  std::string filename;
  std::string output_path;
  std::string absolute_path;
  std::string directory_path;
};

OutputPaths resolve_output_paths(
    const std::string& default_output_dir,
    const std::string& toml_base_dir,
    const std::string& output_option,
    const std::string& title,
    const std::string& dir_name,
    const std::array<std::string, 3>& datetime,
    int date_offset);

}  // namespace radicc
