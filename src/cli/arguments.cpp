// karing-style: namespace, snake_case
#include "cli/arguments.h"
#include <cstdlib>
#include <exception>
#include <iostream>

namespace radicc {

namespace {

void show_usage(const char* program_name) {
  std::cout
      << "Usage: " << program_name << " [options]\n"
      << "Options:\n"
      << "  -t, --target <name>       TOML section name\n"
      << "  -i, --id <id>             TOML id\n"
      << "  -u, --url <url>           Radiko timeshift URL\n"
      << "  -d, --duration <minutes>  Recording duration in minutes\n"
      << "      --date-offset <days>  Shift filename date backward\n"
      << "  -o, --output <path>       Output filename base or explicit path\n"
      << "  -w, --weekday <value>     Reserved option\n"
      << "  -p, --personality <name>  Personality override\n"
      << "      --fetch               Resolve program info without recording\n"
      << "      --json                Print result as JSON\n"
      << "  -h, --help                Show this help\n";
}

}  // namespace

void parse_arguments(
    int argc,
    char* argv[],
    std::string& target,
    std::string& id,
    std::string& url,
    int& duration,
    int& date_offset,
    bool& date_offset_set,
    std::string& output,
    std::string& weekday,
    std::string& personality,
    bool& fetch_only,
    bool& json_output) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      show_usage(argv[0]);
      std::exit(0);
    } else if ((arg == "--target" || arg == "-t") && i + 1 < argc) {
      target = argv[++i];
    } else if ((arg == "--url" || arg == "-u") && i + 1 < argc) {
      url = argv[++i];
    } else if ((arg == "--id" || arg == "-i") && i + 1 < argc) {
      id = argv[++i];
    } else if ((arg == "--duration" || arg == "-d") && i + 1 < argc) {
      try {
        duration = std::stoi(argv[++i]);
      } catch (const std::exception&) {
        std::cerr << "Invalid duration: " << argv[i] << std::endl;
        std::exit(1);
      }
      if (duration <= 0) {
        std::cerr << "Duration must be greater than 0." << std::endl;
        std::exit(1);
      }
    } else if (arg == "--date-offset" && i + 1 < argc) {
      try {
        date_offset = std::stoi(argv[++i]);
        date_offset_set = true;
      } catch (const std::exception&) {
        std::cerr << "Invalid date offset: " << argv[i] << std::endl;
        std::exit(1);
      }
      if (date_offset < 0) {
        std::cerr << "Date offset must be 0 or greater." << std::endl;
        std::exit(1);
      }
    } else if ((arg == "--output" || arg == "-o") && i + 1 < argc) {
      output = argv[++i];
    } else if ((arg == "--weekday" || arg == "-w") && i + 1 < argc) {
      weekday = argv[++i];
    } else if ((arg == "--personality" || arg == "-p") && i + 1 < argc) {
      personality = argv[++i];
    } else if (arg == "--fetch") {
      fetch_only = true;
    } else if (arg == "--json") {
      json_output = true;
    } else {
      std::cerr << "Invalid argument: " << arg << std::endl;
      std::cerr << "Try --help for usage." << std::endl;
      std::exit(1);
    }
  }
}

} // namespace radicc
