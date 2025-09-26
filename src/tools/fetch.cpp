// Simple fetch-and-parse tester for radiko URLs.
// Usage:
//   radicc_fetch <url> [--title "Exact Title"]
// Prints parsed values to stdout (JSON-ish), without recording.

#include <cstdio>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

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

struct Prog { std::string id, title, pfm, ft, to, img; };

static std::vector<Prog> parse_progs_from_xml(const std::string& xml) {
  std::vector<Prog> out;
  std::regex prog_re(R"(<prog\b[^>]*?id=\"([^\"]+)\"[^>]*?ft=\"(\d{14})\"[^>]*?to=\"(\d{14})\"[^>]*?>([\s\S]*?)</prog>)");
  std::regex title_re(R"(<title>([\s\S]*?)</title>)");
  std::regex pfm_re(R"(<pfm>([\s\S]*?)</pfm>)");
  std::regex img_re(R"(<img>([\s\S]*?)</img>)");
  std::smatch m;
  auto begin = xml.cbegin();
  auto end = xml.cend();
  while (std::regex_search(begin, end, m, prog_re)) {
    Prog p; p.id = m[1].str(); p.ft = m[2].str(); p.to = m[3].str();
    const std::string inner = m[4].str();
    std::smatch mt; if (std::regex_search(inner, mt, title_re)) p.title = mt[1].str();
    std::smatch mp; if (std::regex_search(inner, mp, pfm_re))   p.pfm   = mp[1].str();
    std::smatch mi; if (std::regex_search(inner, mi, img_re))   p.img   = mi[1].str();
    out.push_back(std::move(p));
    begin = m.suffix().first;
  }
  return out;
}

static std::optional<std::string> parse_image_from_event_html(const std::string& html) {
  std::smatch m; std::regex img_re(R"(<img[^>]+src=\"([^\"]+)\")");
  if (std::regex_search(html, m, img_re)) return m[1].str();
  return std::nullopt;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: radicc_fetch <url> [--title \"Exact Title\"]\n";
    return 1;
  }
  std::string url = argv[1];
  std::string title;
  for (int i = 2; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--title" && i + 1 < argc) { title = argv[++i]; }
  }

  std::string body = fetch_text(url);
  if (body.empty()) {
    std::cerr << "Fetch failed or empty response" << std::endl;
    return 2;
  }

  // Decide by URL shape
  if (std::regex_search(url, std::regex(R"(/v3/program/station/(date|weekly)/)"))) {
    auto progs = parse_progs_from_xml(body);
    // Print JSON-ish
    if (!title.empty()) {
      bool found = false;
      for (const auto& p : progs) if (p.title == title) {
        std::cout << "{\n"
                  << "  \"id\": \"" << p.id << "\",\n"
                  << "  \"title\": \"" << p.title << "\",\n"
                  << "  \"pfm\": \"" << p.pfm << "\",\n"
                  << "  \"ft\": \"" << p.ft << "\",\n"
                  << "  \"to\": \"" << p.to << "\",\n"
                  << "  \"img\": \"" << p.img << "\"\n"
                  << "}\n";
        found = true; break;
      }
      if (!found) {
        std::cout << "{}\n";
      }
    } else {
      std::cout << "[\n";
      for (size_t i = 0; i < progs.size(); ++i) {
        const auto& p = progs[i];
        std::cout << "  {\n"
                  << "    \"id\": \"" << p.id << "\",\n"
                  << "    \"title\": \"" << p.title << "\",\n"
                  << "    \"pfm\": \"" << p.pfm << "\",\n"
                  << "    \"ft\": \"" << p.ft << "\",\n"
                  << "    \"to\": \"" << p.to << "\",\n"
                  << "    \"img\": \"" << p.img << "\"\n"
                  << "  }" << (i + 1 == progs.size() ? "\n" : ",\n");
      }
      std::cout << "]\n";
    }
    return 0;
  }

  if (std::regex_search(url, std::regex(R"(/mobile/events/\d+)"))) {
    auto img = parse_image_from_event_html(body);
    std::cout << "{\n  \"image\": \"" << (img ? *img : std::string()) << "\"\n}\n";
    return 0;
  }

  std::cerr << "Unrecognized URL shape" << std::endl;
  return 3;
}
