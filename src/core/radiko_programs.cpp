#include "core/radiko_programs.h"
#include <cstdio>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

namespace radicc {

static std::string fetch_text(const std::string& url) {
  std::ostringstream cmd;
  cmd << "curl --silent --fail \"" << url << "\"";
  std::string result;
  FILE* pipe = popen(cmd.str().c_str(), "r");
  if (!pipe) return result;
  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) result.append(buf);
  pclose(pipe);
  return result;
}

std::optional<std::string> find_program_event_url(
    const std::string& station_id,
    const std::string& yyyymmdd,
    const std::string& title) {
  if (station_id.empty() || yyyymmdd.size() != 8 || title.empty()) return std::nullopt;
  const std::string url = "https://radiko.jp/v3/program/station/date/" + yyyymmdd + "/" + station_id + ".xml";
  std::string xml = fetch_text(url);
  if (xml.empty()) return std::nullopt;

  // Very simple extraction: iterate <prog ... id="..." ft="..." to="..."> ... <title>...</title> <pfm>...</pfm> ... </prog>
  // Use non-greedy matches to approximate.
  std::regex prog_re(R"(<prog\b[^>]*?id=\"([^\"]+)\"[^>]*?ft=\"(\d{14})\"[^>]*?to=\"(\d{14})\"[^>]*?>([\s\S]*?)</prog>)");
  std::regex title_re(R"(<title>([\s\S]*?)</title>)");
  std::regex pfm_re(R"(<pfm>([\s\S]*?)</pfm>)");
  std::smatch m;
  auto begin = xml.cbegin();
  auto end = xml.cend();
  while (std::regex_search(begin, end, m, prog_re)) {
    const std::string prog_id = m[1].str();
    const std::string ft = m[2].str();
    const std::string to = m[3].str();
    const std::string inner = m[4].str();
    std::smatch mt;
    if (std::regex_search(inner, mt, title_re)) {
      std::string t = mt[1].str();
      if (t == title) {
        return std::string("https://radiko.jp/mobile/events/") + prog_id;
      }
    }
    begin = m.suffix().first;
  }
  return std::nullopt;
}

std::optional<ProgramEventInfo> find_program_event_info(
    const std::string& station_id,
    const std::string& yyyymmdd,
    const std::string& title) {
  // Re-parse XML to obtain ft/to/pfm while finding event id
  const std::string url = "https://radiko.jp/v3/program/station/date/" + yyyymmdd + "/" + station_id + ".xml";
  std::string xml = fetch_text(url);
  if (xml.empty()) return std::nullopt;

  std::regex prog_re(R"(<prog\b[^>]*?id=\"([^\"]+)\"[^>]*?ft=\"(\d{14})\"[^>]*?to=\"(\d{14})\"[^>]*?>([\s\S]*?)</prog>)");
  std::regex title_re(R"(<title>([\s\S]*?)</title>)");
  std::regex pfm_re(R"(<pfm>([\s\S]*?)</pfm>)");
  std::smatch m;
  auto begin = xml.cbegin();
  auto end = xml.cend();
  ProgramEventInfo info;
  bool found = false;
  while (std::regex_search(begin, end, m, prog_re)) {
    const std::string prog_id = m[1].str();
    const std::string ft = m[2].str();
    const std::string to = m[3].str();
    const std::string inner = m[4].str();
    std::smatch mt;
    if (std::regex_search(inner, mt, title_re)) {
      std::string t = mt[1].str();
      if (t == title) {
        info.event_url = std::string("https://radiko.jp/mobile/events/") + prog_id;
        info.ft = ft;
        info.to = to;
        std::smatch mp;
        if (std::regex_search(inner, mp, pfm_re)) info.pfm = mp[1].str();
        info.title = t;
        found = true;
        break;
      }
    }
    begin = m.suffix().first;
  }
  if (!found) return std::nullopt;

  // Try to fetch image from event page (best-effort)
  std::string html = fetch_text(info.event_url);
  if (!html.empty()) {
    std::smatch m;
    std::regex img_re(R"(<img[^>]+src=\"([^\"]+)\")");
    if (std::regex_search(html, m, img_re)) info.image_url = m[1].str();
  }
  return info;
}

std::optional<ProgramEventInfo> find_program_by_station_ft(
    const std::string& station_id,
    const std::string& ft) {
  if (station_id.empty() || ft.size() != 14) return std::nullopt;
  const std::string url = "https://radiko.jp/v3/program/station/weekly/" + station_id + ".xml";
  std::string xml = fetch_text(url);
  if (xml.empty()) return std::nullopt;
  std::regex prog_re(R"(<prog\b[^>]*?id=\"([^\"]+)\"[^>]*?ft=\"(\d{14})\"[^>]*?to=\"(\d{14})\"[^>]*?>([\s\S]*?)</prog>)");
  std::regex title_re(R"(<title>([\s\S]*?)</title>)");
  std::regex pfm_re(R"(<pfm>([\s\S]*?)</pfm>)");
  std::regex img_re(R"(<img>([\s\S]*?)</img>)");
  std::smatch m;
  auto begin = xml.cbegin();
  auto end = xml.cend();
  while (std::regex_search(begin, end, m, prog_re)) {
    const std::string id = m[1].str();
    const std::string ft_attr = m[2].str();
    const std::string to = m[3].str();
    const std::string inner = m[4].str();
    if (ft_attr == ft) {
      ProgramEventInfo info;
      info.event_url = std::string("https://radiko.jp/mobile/events/") + id;
      info.ft = ft_attr;
      info.to = to;
      std::smatch mt; if (std::regex_search(inner, mt, title_re)) info.title = mt[1].str();
      std::smatch mp; if (std::regex_search(inner, mp, pfm_re)) info.pfm = mp[1].str();
      std::smatch mi; if (std::regex_search(inner, mi, img_re)) info.img = mi[1].str();
      if (!info.img.empty()) info.image_url = info.img; else {
        std::string html = fetch_text(info.event_url);
        if (!html.empty()) {
          std::smatch mh; std::regex im(R"(<img[^>]+src=\"([^\"]+)\")");
          if (std::regex_search(html, mh, im)) info.image_url = mh[1].str();
        }
      }
      return info;
    }
    begin = m.suffix().first;
  }
  return std::nullopt;
}

std::optional<ProgramEventInfo> find_nearest_weekly_program_info(
    const std::string& station_id,
    const std::string& title) {
  if (station_id.empty() || title.empty()) return std::nullopt;
  const std::string url = "https://radiko.jp/v3/program/station/weekly/" + station_id + ".xml";
  std::string xml = fetch_text(url);
  if (xml.empty()) return std::nullopt;

  std::regex prog_re(R"(<prog\b[^>]*?id=\"([^\"]+)\"[^>]*?ft=\"(\d{14})\"[^>]*?to=\"(\d{14})\"[^>]*?>([\s\S]*?)</prog>)");
  std::regex title_re(R"(<title>([\s\S]*?)</title>)");
  std::regex pfm_re(R"(<pfm>([\s\S]*?)</pfm>)");
  std::regex img_re(R"(<img>([\s\S]*?)</img>)");
  std::smatch m;
  auto begin = xml.cbegin();
  auto end = xml.cend();

  // Find all matching titles
  struct Cand { std::string id, ft, to, pfm, img; };
  std::vector<Cand> cands;
  while (std::regex_search(begin, end, m, prog_re)) {
    const std::string id = m[1].str();
    const std::string ft = m[2].str();
    const std::string to = m[3].str();
    const std::string inner = m[4].str();
    std::smatch mt; if (!std::regex_search(inner, mt, title_re)) { begin = m.suffix().first; continue; }
    std::string t = mt[1].str();
    if (t == title) {
      Cand c{ id, ft, to, {}, {} };
      std::smatch mp; if (std::regex_search(inner, mp, pfm_re)) c.pfm = mp[1].str();
      std::smatch mi; if (std::regex_search(inner, mi, img_re)) c.img = mi[1].str();
      cands.push_back(std::move(c));
    }
    begin = m.suffix().first;
  }
  if (cands.empty()) return std::nullopt;

  // Choose nearest past (to <= now). Compare lexicographically (yyyyMMddHHmmss)
  auto now = [](){
    char buf[16];
    std::time_t t = std::time(nullptr);
    std::tm* lt = std::localtime(&t);
    std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", lt);
    return std::string(buf);
  }();

  Cand* best = nullptr;
  for (auto& c : cands) {
    if (c.to <= now) {
      if (!best || c.to > best->to) best = &c; // latest past
    }
  }
  if (!best) return std::nullopt;

  ProgramEventInfo info;
  info.event_url = std::string("https://radiko.jp/mobile/events/") + best->id;
  info.ft = best->ft;
  info.to = best->to;
  info.pfm = best->pfm;
  info.img = best->img;
  // Fallback to fetch event page image
  if (!info.img.empty()) info.image_url = info.img; else {
    std::string html = fetch_text(info.event_url);
    if (!html.empty()) {
      std::smatch mh; std::regex im(R"(<img[^>]+src=\"([^\"]+)\")");
      if (std::regex_search(html, mh, im)) info.image_url = mh[1].str();
    }
  }
  return info;
}

} // namespace radicc
