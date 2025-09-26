#pragma once
#include <string>

namespace radicc {

bool login_to_radiko(const std::string& mail, const std::string& password, std::string& session_id);
bool authorize_radiko(std::string& authtoken, const std::string& session_id);
bool record_radiko(const std::string& station_id, const std::string& fromtime,
                   const std::string& totime, const std::string& filename,
                   const std::string& authtoken,
                   const std::string& pfm, const std::string& album_title,
                   const std::string& dir_name, const std::string& outputDir,
                   const std::string& image_url = std::string());
void logout_from_radiko(const std::string& radiko_session);

} // namespace radicc
