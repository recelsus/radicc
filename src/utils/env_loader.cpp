#include "utils/env_loader.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

namespace radicc {

bool file_exists(const std::string& path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0);
}

std::string get_config_path_for_env(const std::string& filename) {
  const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
  std::string config_path;
  if (xdg_config_home) {
    config_path = std::string(xdg_config_home) + "/radicc";
  } else {
    const char* home = std::getenv("HOME");
    if (home) config_path = std::string(home) + "/.config/radicc";
    else { std::cerr << "Error: Could not determine home directory.\n"; std::exit(1); }
  }
  return config_path + "/" + filename;
}

void load_env_file(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) { std::cerr << "Error: Could not open " << filepath << std::endl; return; }
  std::string line;
  while (std::getline(file, line)) {
    size_t pos = line.find('=');
    if (pos == std::string::npos) continue;
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);
    setenv(key.c_str(), value.c_str(), 1);
  }
}

void load_env_from_file() {
  std::string env_path = get_config_path_for_env("env");
  std::string dotenv_path = get_config_path_for_env(".env");
  struct stat st;
  const bool suppress_logs = std::getenv("RADICC_SUPPRESS_ENV_LOG") != nullptr;
  bool loaded = false;

  if (stat(env_path.c_str(), &st) == 0) {
    if (!suppress_logs) {
      std::cerr << "Loading from env file: " << env_path << std::endl;
    }
    load_env_file(env_path);
    loaded = true;
  }

  if (stat(dotenv_path.c_str(), &st) == 0) {
    if (!suppress_logs) {
      std::cerr << "Loading from .env file: " << dotenv_path << std::endl;
    }
    load_env_file(dotenv_path);
    loaded = true;
  }

  if (!loaded && !suppress_logs) {
    std::cerr << "Note: No env file found; relying on environment variables only.\n";
  }
}

bool check_radiko_credentials(std::string& radiko_user, std::string& radiko_pass, std::string& output_dir) {
  const char* home = std::getenv("HOME");
  const auto expand_path = [&](const std::string& raw) -> std::string {
    std::string expanded = raw;
    expanded.erase(std::remove(expanded.begin(), expanded.end(), '\"'), expanded.end());

    if (home) {
      size_t pos = expanded.find("$HOME");
      if (pos != std::string::npos) expanded.replace(pos, 5, home);
      if (!expanded.empty() && expanded.front() == '~') {
        expanded.replace(0, 1, home);
      }
    }

    if (!expanded.empty() && expanded.back() != '/') {
      expanded += '/';
    }

    return expanded;
  };

  const char* radicc_output = std::getenv("RADICC_OUTPUT_DIR");
  const char* legacy_output = std::getenv("OUTPUT_DIR");
  if (radicc_output && *radicc_output) {
    output_dir = expand_path(radicc_output);
  } else if (legacy_output && *legacy_output) {
    output_dir = expand_path(legacy_output);
  } else {
    output_dir = "./";
  }

  const char* user = std::getenv("RADIKO_USER");
  const char* pass = std::getenv("RADIKO_PASS");
  if (user && *user && pass && *pass) {
    radiko_user = user;
    radiko_pass = pass;
    return true;
  }

  radiko_user.clear();
  radiko_pass.clear();
  return false;
}

} // namespace radicc
