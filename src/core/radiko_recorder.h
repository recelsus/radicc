#pragma once
#include "core/radiko_stream.h"

#include <string>

namespace radicc {

bool record_radiko(const RadikoStreamPlan& stream_plan, const std::string& filename,
                   const std::string& pfm, const std::string& album_title,
                   const std::string& dir_name, const std::string& outputDir,
                   const std::string& image_url = std::string());

} // namespace radicc
