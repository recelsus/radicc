#include "core/radiko_programs_xml.h"

#include "core/radiko_http.h"

#include <cctype>
#include <optional>
#include <regex>

namespace radicc {
namespace {

std::optional<std::string> find_attribute(
    const std::string& open_tag,
    const std::string& name) {
  const std::string needle = name + "=\"";
  std::size_t position = 0;
  while ((position = open_tag.find(needle, position)) != std::string::npos) {
    if (position == 0 || std::isspace(static_cast<unsigned char>(open_tag[position - 1]))) {
      const std::size_t value_start = position + needle.size();
      const std::size_t value_end = open_tag.find('"', value_start);
      if (value_end != std::string::npos) {
        return open_tag.substr(value_start, value_end - value_start);
      }
      return std::nullopt;
    }
    position += needle.size();
  }
  return std::nullopt;
}

std::string find_element_text(const std::string& body, const std::string& name) {
  const std::string open_tag = "<" + name + ">";
  const std::string close_tag = "</" + name + ">";
  const std::size_t value_start = body.find(open_tag);
  if (value_start == std::string::npos) return {};
  const std::size_t content_start = value_start + open_tag.size();
  const std::size_t value_end = body.find(close_tag, content_start);
  if (value_end == std::string::npos) return {};
  return body.substr(content_start, value_end - content_start);
}

}  // namespace

std::string fetch_programs_xml(const std::string& url) {
  auto result = curl_get_text(url);
  return result ? *result : std::string();
}

std::vector<ProgramEventInfo> parse_programs_from_xml(const std::string& xml) {
  std::vector<ProgramEventInfo> programs;
  std::size_t position = 0;
  while ((position = xml.find("<prog ", position)) != std::string::npos) {
    const std::size_t open_end = xml.find('>', position);
    if (open_end == std::string::npos) break;
    const std::size_t close_start = xml.find("</prog>", open_end + 1);
    if (close_start == std::string::npos) break;

    const std::string open_tag = xml.substr(position, open_end - position + 1);
    const auto id = find_attribute(open_tag, "id");
    const auto ft = find_attribute(open_tag, "ft");
    const auto to = find_attribute(open_tag, "to");
    if (!id || !ft || !to || ft->size() != 14 || to->size() != 14) {
      position = close_start + 7;
      continue;
    }

    const std::string body = xml.substr(open_end + 1, close_start - open_end - 1);
    ProgramEventInfo info;
    info.event_url = std::string("https://radiko.jp/mobile/events/") + *id;
    info.ft = *ft;
    info.to = *to;
    info.title = find_element_text(body, "title");
    info.pfm = find_element_text(body, "pfm");
    info.img = info.image_url = find_element_text(body, "img");
    programs.push_back(std::move(info));
    position = close_start + 7;
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
