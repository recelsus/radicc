#ifndef ARGUMENTS_HANDLER_H
#define ARGUMENTS_HANDLER_H

#include <string>

void parseArguments(int argc, char* argv[], std::string& target, std::string& url, 
                    int& duration, std::string& output);

#endif

