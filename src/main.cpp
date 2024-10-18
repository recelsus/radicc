#include <iostream>
#include <string>
#include <array>
#include <tuple>
#include "env_loader.h"
#include "url_parser.h"
#include "date_handler.h"
#include "toml_parser.h"
#include "env_loader.h"
#include "arguments_handler.h"
#include "radiko_recorder.h"

const std::string APP_NAME = "radicc";

void printErrorAndExit(const std::string& message) {
  std::cerr << "Error: " << message << std::endl;
  std::exit(1);
}

void validateVariables(const std::string& station_id, const std::string& startTime, 
                       const std::string& endTime, const std::string& output, int duration) {
  if (station_id.empty()) printErrorAndExit("Station ID is required.");
  if (startTime.empty()) printErrorAndExit("Start time is required.");
  if (endTime.empty()) printErrorAndExit("End time is required.");
  if (duration == 0) printErrorAndExit("Duration must be greater than 0.");
}

int main(int argc, char* argv[]) {
  std::string target, url, station_id, output, personality, weekday, organize;
  bool dryrun = false;
  int duration = 0;
  std::array<std::string, 3> datetime = { "", "", "" };

  parseArguments(argc, argv, target, url, duration, output, weekday, personality, dryrun);

  bool hasValidConfig = false;

  if (!target.empty()) {
    auto config = parseTOML(target);

    if(!config.empty()) {
      hasValidConfig = true;
      station_id = config["station"];
      std::string day = weekday.empty() ? config["day"] : weekday;
      std::string time = config["time"];
      output = output.empty() ? config["output"] : output;
      duration = (duration == 0) ? std::stoi(config["duration"]) : duration;
      personality = !personality.empty() ? personality : (config.count("personality") ? config["personality"] : "");

      organize = config.count("organize") ? config["organize"] : "";

      std::string targetDate = getLatestDateForDay(day);
      datetime = { targetDate.substr(0, 4), targetDate.substr(4, 4), time };
    } else {
      std::cerr << "Warning: TOML section not found or TOML file not available. Proceeding with command-line arguments" << std::endl;
    }
  }

  if (!url.empty()) {
    auto result = parseRadikoURL(url);
    if (!result) {
      printErrorAndExit("Invalid URL format.");
    }
    std::tie(station_id, datetime) = *result;
    hasValidConfig = true;
  }

  if (!hasValidConfig) {
    printErrorAndExit("Missing required configuration. Provide either valid TOML config or URL.");
  }

  std::string startTime = generate14DigitDatetime(datetime, 0);
  std::string endTime = generate14DigitDatetime(datetime, duration);

  if (output.empty()) {
    output = "radiko_output.m4a";
  }

  validateVariables(station_id, startTime, endTime, output, duration);

  loadEnvFromFile();

  std::string radikoUser, radikoPass;
  std::string outputDir;

  if (!checkRadikoCredentials(radikoUser, radikoPass, outputDir)) {
    std::cerr << "No Radiko credentials found. Proceeding without login." << std::endl;
  }

  output = replacePlaceholders(output, startTime.substr(0, 8), startTime.substr(8, 4));
  
  std::string authtoken;
  std::string session_id;
  
  bool loggedIn = false;
  if (!radikoUser.empty() && !radikoPass.empty()) {
    loggedIn = loginToRadiko(radikoUser, radikoPass, session_id);
    
    if (!loggedIn) {
      std::cerr << "Warning: Login failed, proceeding without Radiko Premium access." << std::endl;
    }
  } else {
    std::cerr << "No Radiko credentials provied, proceeding without Radiko Premium access." << std::endl;
  }

  if (dryrun == false) {
    if (!authorizeRadiko(authtoken, session_id)) {
      printErrorAndExit("Authorization failed.");
    }

    if (!recordRadiko(station_id, startTime, endTime, output, authtoken, personality, organize, outputDir)) {
      printErrorAndExit("Failed to record the broadcast.");
    }

    std::string session;
    logoutFromRadiko(session);

  } else if (dryrun == true) {
    std::cout << "--dry-run was specified, not execute." << std::endl;
  }

  std::cout << std::endl;
  std::cout << "Station: " << station_id << std::endl;
  std::cout << "Time: " << startTime << " - " << endTime << std::endl;
  std::cout << "Duration: " << duration << " minutes" << std::endl;
  std::cout << "Output File: " << output << std::endl;
  std::cout << "Personality: " << personality << std::endl;
  std::cout << "Organize: " << organize << std::endl;
  std::cout << std::endl;

  std::cout << "Recording completed successfully.\n";

  return 0;
}

