#include "core/radiko_stream.h"

#include "core/radiko_http.h"
#include "utils/date.h"

#include <cstdint>
#include <optional>
#include <random>
#include <regex>
#include <sstream>

namespace radicc {
namespace {

std::string generate_lsid() {
  static constexpr char kHex[] = "0123456789abcdef";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0, 15);

  std::string value;
  value.reserve(32);
  for (int i = 0; i < 32; ++i) value.push_back(kHex[dist(gen)]);
  return value;
}

std::vector<std::string> extract_playlist_create_urls(
    const std::string& xml,
    bool areafree) {
  std::vector<std::string> urls;
  const std::string needle = areafree ? "areafree=\"1\"" : "areafree=\"0\"";
  std::regex url_re(R"(<url\b([^>]*)>([\s\S]*?)</url>)");
  std::regex playlist_re(R"(<playlist_create_url>([\s\S]*?)</playlist_create_url>)");
  std::smatch m;
  auto begin = xml.cbegin();
  auto end = xml.cend();
  while (std::regex_search(begin, end, m, url_re)) {
    const std::string attrs = m[1].str();
    const std::string inner = m[2].str();
    if (attrs.find("timefree=\"1\"") == std::string::npos || attrs.find(needle) == std::string::npos) {
      begin = m.suffix().first;
      continue;
    }
    std::smatch mp;
    if (std::regex_search(inner, mp, playlist_re)) {
      urls.push_back(trim_crlf(mp[1].str()));
    }
    begin = m.suffix().first;
  }
  return urls;
}

int compute_chunk_length(int remaining_seconds) {
  if (remaining_seconds >= 300) return 300;
  if (remaining_seconds % 5 == 0) return remaining_seconds;
  return ((remaining_seconds / 5) + 1) * 5;
}

}  // namespace

std::optional<RadikoStreamPlan> build_timefree_stream_plan(
    const std::string& station_id,
    const std::string& fromtime,
    const std::string& totime,
    bool is_areafree,
    const RadikoAuthState& auth_state) {
  if (station_id.empty() || !is_valid_datetime14(fromtime) || !is_valid_datetime14(totime)) {
    return std::nullopt;
  }

  const auto xml = curl_get_text("https://radiko.jp/v3/station/stream/pc_html5/" + station_id + ".xml");
  if (!xml) return std::nullopt;

  const auto playlist_urls = extract_playlist_create_urls(*xml, is_areafree);
  if (playlist_urls.empty()) return std::nullopt;

  const std::int64_t start_unixtime = to_unixtime_jst(fromtime);
  const std::int64_t end_unixtime = to_unixtime_jst(totime);
  if (start_unixtime < 0 || end_unixtime <= start_unixtime) return std::nullopt;

  RadikoStreamPlan plan;
  plan.request_headers = "X-Radiko-Authtoken: " + auth_state.authtoken + "\r\n"
      + "X-Radiko-AreaId: " + auth_state.area_id;

  const std::string lsid = generate_lsid();
  for (const auto& base_url : playlist_urls) {
    RadikoStreamSource source;
    std::int64_t seek_timestamp = start_unixtime;
    int remaining_seconds = static_cast<int>(end_unixtime - start_unixtime);
    while (remaining_seconds > 0) {
      const int chunk_length = compute_chunk_length(remaining_seconds);
      const std::string seek = from_unixtime_jst(seek_timestamp);
      const std::string end_at = from_unixtime_jst(seek_timestamp + chunk_length);
      if (seek.empty() || end_at.empty()) return std::nullopt;

      std::ostringstream url;
      url << base_url
          << "?station_id=" << station_id
          << "&start_at=" << fromtime
          << "&ft=" << fromtime
          << "&seek=" << seek
          << "&end_at=" << end_at
          << "&to=" << end_at
          << "&l=" << chunk_length
          << "&lsid=" << lsid
          << "&type=c";
      source.chunks.push_back({url.str(), chunk_length});
      seek_timestamp += chunk_length;
      remaining_seconds -= chunk_length;
    }
    plan.sources.push_back(std::move(source));
  }

  return plan;
}

}  // namespace radicc
