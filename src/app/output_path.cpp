#include "app/output_path.h"

#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace radicc {
namespace {

bool has_path_separator(const std::string& value) {
  return value.find('/') != std::string::npos || value.find('\\') != std::string::npos;
}

bool is_directory_output_override(const std::string& value) {
  if (value.empty()) return false;
  if (value == "." || value == ".." || value == "./" || value == "../") return true;
  return value.back() == '/' || value.back() == '\\';
}

std::string ensure_trailing_separator(const std::filesystem::path& path) {
  std::string value = path.empty() ? std::string("./") : path.string();
  if (value.empty()) value = "./";
  if (value.back() != '/') value += '/';
  return value;
}

std::string expand_output_dir(std::string value) {
  value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
  const char* home = std::getenv("HOME");
  if (home) {
    std::size_t pos = value.find("$HOME");
    if (pos != std::string::npos) value.replace(pos, 5, home);
    if (!value.empty() && value.front() == '~') value.replace(0, 1, home);
  }
  return ensure_trailing_separator(std::filesystem::path(value));
}

std::string build_output_path(const std::string& output_dir, const std::string& dir_name, const std::string& filename) {
  return dir_name.empty() ? (output_dir + filename) : (output_dir + dir_name + "/" + filename);
}

std::string build_filename(
    const std::string& output_option,
    const std::string& title,
    const std::array<std::string, 3>& datetime,
    int date_offset,
    bool explicit_output_path) {
  auto ymd = datetime[0] + datetime[1];
  if (ymd.size() == 8 && date_offset != 0) {
    std::tm t{};
    std::istringstream ss(ymd);
    ss >> std::get_time(&t, "%Y%m%d");
    t.tm_mday -= date_offset;
    std::mktime(&t);
    std::ostringstream os;
    os << std::put_time(&t, "%Y%m%d");
    ymd = os.str();
  }
  const std::string base = output_option.empty() || explicit_output_path ? title : output_option;
  return base + "-" + ymd + ".m4a";
}

}  // namespace

OutputPaths resolve_output_paths(
    const std::string& default_output_dir,
    const std::string& toml_base_dir,
    const std::string& output_option,
    const std::string& title,
    const std::string& dir_name,
    const std::array<std::string, 3>& datetime,
    int date_offset) {
  const bool explicit_output_path = has_path_separator(output_option) || is_directory_output_override(output_option);

  OutputPaths paths;
  paths.output_dir = default_output_dir;
  if (!toml_base_dir.empty() && !explicit_output_path) paths.output_dir = expand_output_dir(toml_base_dir);
  paths.dir_name = dir_name;
  paths.filename = build_filename(output_option, title, datetime, date_offset, explicit_output_path);

  if (!output_option.empty() && explicit_output_path) {
    const std::filesystem::path override_path(output_option);
    paths.dir_name.clear();
    if (is_directory_output_override(output_option)) {
      paths.output_dir = ensure_trailing_separator(override_path);
    } else {
      paths.output_dir = ensure_trailing_separator(
          override_path.has_parent_path() ? override_path.parent_path() : std::filesystem::path("."));
      paths.filename = override_path.filename().string();
    }
  }

  paths.output_path = build_output_path(paths.output_dir, paths.dir_name, paths.filename);
  paths.absolute_path = std::filesystem::absolute(paths.output_path).string();
  paths.directory_path = paths.dir_name.empty() ? paths.output_dir : paths.output_dir + paths.dir_name + "/";
  return paths;
}

}  // namespace radicc
