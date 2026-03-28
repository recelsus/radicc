#include "app/command_options.h"
#include "app/list_command.h"
#include "app/record_command.h"
#include "cli/arguments.h"

#include <iostream>
#include <string>

namespace {

int print_missing_subcommand(const std::string& program_name) {
  std::cerr << "Subcommand is required. Use one of: rec, fetch, list.\n";
  std::cerr << "Examples:\n";
  std::cerr << "  " << program_name << " rec --url https://radiko.jp/#!/ts/JORF/20260322003000\n";
  std::cerr << "  " << program_name << " fetch -t karin-zatsudan\n";
  std::cerr << "  " << program_name << " list --station-id JORF\n";
  std::cerr << "Run `" << program_name << " --help` for more information.\n";
  return 1;
}

}  // namespace

int main(int argc, char* argv[]) {
  using namespace radicc;
  const std::string program_name = argc > 0 ? std::string(argv[0]) : std::string("radicc");
  if (argc < 2) return print_missing_subcommand(program_name);

  const std::string command = argv[1];
  if (command == "--help" || command == "-h") {
    show_usage(program_name);
    return 0;
  }
  if (command != "rec" && command != "fetch" && command != "list") {
    std::cerr << "Error: unknown command '" << command << "'.\n";
    std::cerr << "Try --help for usage.\n";
    return 1;
  }

  CommandOptions options;
  options.fetch_only = command == "fetch";
  parse_arguments(program_name, command, argc, argv, 2, options);

  if (command == "list") return run_list_command(options);
  return run_record_command(options);
}
