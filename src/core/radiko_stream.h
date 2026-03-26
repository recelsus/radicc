#pragma once

#include "core/radiko_auth.h"

#include <optional>
#include <string>
#include <vector>

namespace radicc {

struct RadikoStreamChunk {
  std::string url;
  int duration_seconds = 0;
};

struct RadikoStreamSource {
  std::vector<RadikoStreamChunk> chunks;
};

struct RadikoStreamPlan {
  std::string request_headers;
  std::vector<RadikoStreamSource> sources;
};

std::optional<RadikoStreamPlan> build_timefree_stream_plan(
    const std::string& station_id,
    const std::string& fromtime,
    const std::string& totime,
    bool is_areafree,
    const RadikoAuthState& auth_state);

}  // namespace radicc
