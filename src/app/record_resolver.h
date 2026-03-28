#pragma once

#include "app/command_options.h"

#include <array>
#include <string>

namespace radicc {

struct ResolvedRecord {
  std::string station_id;
  std::string title;
  std::string pfm;
  std::string image_url;
  std::string dir_name;
  std::string toml_base_dir;
  std::array<std::string, 3> datetime = {"", "", ""};
  int duration = 0;
  int date_offset = 0;
  bool json_output = false;
  bool fetch_only = false;
};

ResolvedRecord resolve_record_command(const CommandOptions& options, int max_timefree_days);

}  // namespace radicc
