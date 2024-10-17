#ifndef RADIKO_RECORDER_H
#define RADIKO_RECORDER_H

#include <string>

bool loginToRadiko(const std::string& mail, const std::string& password, std::string& session_id);
bool authorizeRadiko(std::string& authtoken, const std::string& session_id);
bool recordRadiko(const std::string& station_id, const std::string& fromtime, 
                  const std::string& totime, const std::string& output, 
                  const std::string& authtoken,
                  const std::string& personality, const std::string& organize);
void logoutFromRadiko(const std::string& radiko_session);

#endif

