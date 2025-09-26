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
  if (stat(env_path.c_str(), &st) == 0) {
    std::cout << "Loading from env file: " << env_path << std::endl;
    load_env_file(env_path);
  } else if (stat(dotenv_path.c_str(), &st) == 0) {
    std::cout << "Loading from .env file: " << dotenv_path << std::endl;
    load_env_file(dotenv_path);
  } else {
    std::cerr << "Error: Neither env nor .env file found.\n";
  }
}

bool check_radiko_credentials(std::string& radikoUser, std::string& radikoPass, std::string& outputDir) {
  const char* user = std::getenv("RADIKO_USER");
  const char* pass = std::getenv("RADIKO_PASS");
  const char* outputBase = std::getenv("OUTPUT_DIR");
  std::string homeDir = std::getenv("HOME");
  if (outputBase) {
    outputDir = std::string(outputBase);
    outputDir.erase(std::remove(outputDir.begin(), outputDir.end(), '\"'), outputDir.end());
    size_t pos = outputDir.find("$HOME");
    if (pos != std::string::npos) outputDir.replace(pos, 5, homeDir);
    if (outputDir.back() != '/') outputDir += '/';
  } else {
    outputDir = "./";
  }
  if (user && pass) { radikoUser = user; radikoPass = pass; return true; }
  return false;
}

} // namespace radicc

