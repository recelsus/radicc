#include "core/radiko_programs_xml.h"

#include "core/radiko_http.h"

#include <regex>

namespace radicc {

std::string fetch_programs_xml(const std::string& url) {
  auto result = curl_get_text(url);
  return result ? *result : std::string();
}

std::vector<ProgramEventInfo> parse_programs_from_xml(const std::string& xml) {
  std::vector<ProgramEventInfo> programs;
  std::regex prog_re(R"(<prog\b[^>]*?id=\"([^\"]+)\"[^>]*?ft=\"(\d{14})\"[^>]*?to=\"(\d{14})\"[^>]*?>([\s\S]*?)</prog>)");
  std::regex title_re(R"(<title>([\s\S]*?)</title>)");
  std::regex pfm_re(R"(<pfm>([\s\S]*?)</pfm>)");
  std::regex img_re(R"(<img>([\s\S]*?)</img>)");
  std::smatch m;
  auto begin = xml.cbegin();
  auto end = xml.cend();
  while (std::regex_search(begin, end, m, prog_re)) {
    ProgramEventInfo info;
    info.event_url = std::string("https://radiko.jp/mobile/events/") + m[1].str();
    info.ft = m[2].str();
    info.to = m[3].str();
    const std::string inner = m[4].str();
    std::smatch mt; if (std::regex_search(inner, mt, title_re)) info.title = mt[1].str();
    std::smatch mp; if (std::regex_search(inner, mp, pfm_re)) info.pfm = mp[1].str();
    std::smatch mi; if (std::regex_search(inner, mi, img_re)) info.img = info.image_url = mi[1].str();
    programs.push_back(std::move(info));
    begin = m.suffix().first;
  }
  return programs;
}

void fill_program_image_from_event_page(ProgramEventInfo& info) {
  if (!info.img.empty()) {
    info.image_url = info.img;
    return;
  }
  const std::string html = fetch_programs_xml(info.event_url);
  if (html.empty()) return;
  std::smatch m;
  std::regex img_re(R"(<img[^>]+src=\"([^\"]+)\")");
  if (std::regex_search(html, m, img_re)) info.image_url = m[1].str();
}

}  // namespace radicc
