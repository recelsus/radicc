// karing-style: namespace, snake_case
#include "cli/arguments.h"
#include <cstdlib>
#include <exception>
#include <iostream>

namespace radicc {

void show_usage(const std::string& program_name, const std::string& command) {
  if (command == "rec") {
    std::cout
        << "Usage: " << program_name << " rec [options]\n"
        << "Options:\n"
        << "  -t, --target <name>       TOML section name\n"
        << "  -i, --id <id>             TOML id\n"
        << "  -u, --url <url>           Radiko timeshift URL\n"
        << "  -d, --duration <minutes>  Recording duration in minutes\n"
        << "      --date-offset <days>  Shift filename date backward\n"
        << "  -o, --output <path>       Output filename base or explicit path\n"
        << "      --json                Print result as JSON\n"
        << "  -h, --help                Show this help\n";
    return;
  }

  if (command == "fetch") {
    std::cout
        << "Usage: " << program_name << " fetch [options]\n"
        << "Options:\n"
        << "  -t, --target <name>       TOML section name\n"
        << "  -i, --id <id>             TOML id\n"
        << "  -u, --url <url>           Radiko timeshift URL\n"
        << "  -d, --duration <minutes>  Recording duration in minutes\n"
        << "      --date-offset <days>  Shift filename date backward\n"
        << "  -o, --output <path>       Output filename base or explicit path\n"
        << "      --json                Print result as JSON\n"
        << "  -h, --help                Show this help\n";
    return;
  }

  if (command == "list") {
    std::cout
        << "Usage: " << program_name << " list --station-id <id> [options]\n"
        << "Options:\n"
        << "      --station-id <id>     Fetch schedule for station id\n"
        << "      --date <YYYYMMDD>     Schedule date (default: today JST)\n"
        << "      --json                Print result as JSON\n"
        << "  -h, --help                Show this help\n";
    return;
  }

  std::cout
      << "Usage: " << program_name << " <command> [options]\n"
      << "Commands:\n"
      << "  rec                     Record program\n"
      << "  fetch                   Resolve program info without recording\n"
      << "  list                    List station schedule for one date\n"
      << "\n"
      << "Examples:\n"
      << "  " << program_name << " rec --url https://radiko.jp/#!/ts/JORF/20260322003000\n"
      << "  " << program_name << " fetch -t karin-zatsudan\n"
      << "  " << program_name << " list --station-id JORF --date 20260322\n"
      << "\n"
      << "Use `<command> --help` for command-specific options.\n";
}

void parse_arguments(
    const std::string& program_name,
    const std::string& command,
    int argc,
    char* argv[],
    int start_index,
    CommandOptions& options) {
  for (int i = start_index; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      show_usage(program_name, command);
      std::exit(0);
    } else if ((arg == "--target" || arg == "-t") && i + 1 < argc) {
      options.target = argv[++i];
    } else if ((arg == "--url" || arg == "-u") && i + 1 < argc) {
      options.url = argv[++i];
    } else if (arg == "--station-id" && i + 1 < argc) {
      options.station_id = argv[++i];
    } else if (arg == "--date" && i + 1 < argc) {
      options.date = argv[++i];
    } else if ((arg == "--id" || arg == "-i") && i + 1 < argc) {
      options.id = argv[++i];
    } else if ((arg == "--duration" || arg == "-d") && i + 1 < argc) {
      try {
        options.duration = std::stoi(argv[++i]);
      } catch (const std::exception&) {
        std::cerr << "Invalid duration: " << argv[i] << std::endl;
        std::exit(1);
      }
      if (options.duration <= 0) {
        std::cerr << "Duration must be greater than 0." << std::endl;
        std::exit(1);
      }
    } else if (arg == "--date-offset" && i + 1 < argc) {
      try {
        options.date_offset = std::stoi(argv[++i]);
        options.date_offset_set = true;
      } catch (const std::exception&) {
        std::cerr << "Invalid date offset: " << argv[i] << std::endl;
        std::exit(1);
      }
      if (options.date_offset < 0) {
        std::cerr << "Date offset must be 0 or greater." << std::endl;
        std::exit(1);
      }
    } else if ((arg == "--output" || arg == "-o") && i + 1 < argc) {
      options.output = argv[++i];
    } else if ((arg == "--weekday" || arg == "-w") && i + 1 < argc) {
      options.weekday = argv[++i];
    } else if ((arg == "--personality" || arg == "-p") && i + 1 < argc) {
      options.personality = argv[++i];
    } else if (arg == "--json") {
      options.json_output = true;
    } else {
      std::cerr << "Invalid argument: " << arg << std::endl;
      std::cerr << "Try --help for usage." << std::endl;
      std::exit(1);
    }
  }
}

} // namespace radicc
