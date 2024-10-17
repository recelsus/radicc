#ifndef ENV_LOADER_H
#define ENV_LOADER_H

#include <string>

bool fileExists(const std::string& path);
std::string getConfigPathForEnv(const std::string& filename);
void loadEnvFile(const std::string& filepath);
void loadEnvFromFile();
bool checkRadikoCredentials(std::string& radikoUser, std::string& radikoPass, std::string& outputDir);

#endif
