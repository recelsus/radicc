#pragma once

#include <string>

namespace radicc {

struct CommandOptions {
  std::string target;
  std::string id;
  std::string url;
  std::string station_id;
  std::string date;
  std::string output;
  std::string weekday;
  std::string personality;
  bool json_output = false;
  bool fetch_only = false;
  bool date_offset_set = false;
  int duration = 0;
  int date_offset = 0;
};

}  // namespace radicc
