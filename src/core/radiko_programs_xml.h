#pragma once

#include "core/radiko_programs.h"

#include <string>
#include <vector>

namespace radicc {

std::string fetch_programs_xml(const std::string& url);
std::vector<ProgramEventInfo> parse_programs_from_xml(const std::string& xml);
void fill_program_image_from_event_page(ProgramEventInfo& info);

}  // namespace radicc
