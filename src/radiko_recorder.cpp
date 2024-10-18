#include "radiko_recorder.h"
#include "base64.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>

std::string generatePartialKey(const std::string& authkey, int keyoffset, int keylength) {

    std::string keySegment = authkey.substr(keyoffset, keylength);
    std::string partialKey = base64::to_base64(keySegment);

    return partialKey;
}

bool loginToRadiko(const std::string& mail, const std::string& password, std::string& session_id) {

    if (mail.empty() || password.empty()) {
        std::cerr << "Warning: No Radiko credentials provided. Skipping login.\n";
        return false;
    }

    std::ostringstream curl_command;
    curl_command << "curl --silent --request POST "
                 << "--data-urlencode \"mail=" << mail << "\" "
                 << "--data-urlencode \"pass=" << password << "\" "
                 << "\"https://radiko.jp/v4/api/member/login\"";

    FILE* pipe = popen(curl_command.str().c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Failed to execute login command.\n";
        return false;
    }

    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    std::size_t pos = result.find("\"radiko_session\":\"");
    if (pos == std::string::npos) {
        std::cerr << "Login failed: Radiko session not found.\n";
        return false;
    }

    session_id = result.substr(pos + 18, result.find("\"", pos + 18) - (pos + 18));
    return true;
}

bool authorizeRadiko(std::string& authtoken, const std::string& session_id) {

    std::string auth1_command =
        "curl --silent "
        "--header \"X-Radiko-App: pc_html5\" "
        "--header \"X-Radiko-App-Version: 0.0.1\" "
        "--header \"X-Radiko-Device: pc\" "
        "--header \"X-Radiko-User: dummy_user\" "
        "--dump-header - "
        "--output /dev/null "
        "\"https://radiko.jp/v2/api/auth1\"";

    FILE* pipe = popen(auth1_command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Failed to execute auth1 command.\n";
        return false;
    }

    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    size_t token_pos = result.find("x-radiko-authtoken: ");
    size_t offset_pos = result.find("x-radiko-keyoffset: ");
    size_t length_pos = result.find("x-radiko-keylength: ");

    if (token_pos == std::string::npos || offset_pos == std::string::npos || length_pos == std::string::npos) {
        std::cerr << "Authorization failed: Required fields not found.\n";
        return false;
    }

    authtoken = result.substr(token_pos + 20, result.find("\n", token_pos) - (token_pos + 20));
    std::string keyOffsetStr = result.substr(offset_pos + 20, result.find("\n", offset_pos) - (offset_pos + 20));
    std::string keyLengthStr = result.substr(length_pos + 20, result.find("\n", length_pos) - (length_pos + 20));

    int keyoffset = std::stoi(keyOffsetStr);
    int keylength = std::stoi(keyLengthStr);

    std::string partialkey = generatePartialKey("bcd151073c03b352e1ef2fd66c32209da9ca0afa", keyoffset, keylength);

    std::string auth2_command =
        "curl --silent --header \"X-Radiko-Device: pc\" "
        "--header \"X-Radiko-User: dummy_user\" "
        "--header \"X-Radiko-AuthToken: " + authtoken + "\" "
        "--header \"X-Radiko-PartialKey: " + partialkey + "\" "
        "--output /dev/null \"https://radiko.jp/v2/api/auth2?radiko_session=" + session_id + "\"";

    int auth2_result = std::system(auth2_command.c_str());

    return auth2_result == 0;
}

bool mkdir_p(const std::string& path, mode_t mode = 0755) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }

    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        std::string parent = path.substr(0, pos);
        if (!parent.empty() && !mkdir_p(parent, mode)) {
            return false;
        }
    }

    if (mkdir(path.c_str(), mode) != 0) {
        if (errno != EEXIST) {
            std::cerr << "Error: Failed to create directory " << path << ": " << strerror(errno) << std::endl;
            return false;
        }
    }
    return true;
}

bool recordRadiko(const std::string& station_id, const std::string& fromtime,
                  const std::string& totime, const std::string& output,
                  const std::string& authtoken,
                  const std::string& personality, const std::string& organize,
                  const std::string& outputDir) {

    std::string artist = personality.empty() ? "\"\"" : "\"" + personality + "\"";
    std::string album = organize.empty() ? "\"\"" : "\"" + organize + "\"";
    
    std::string outputPath;
    if (!organize.empty()) {
        outputPath = outputDir + organize + "/" + output;
        if (!mkdir_p(outputDir + organize, 0755)) {
          std::cerr << "Error: Failed to create directory " << outputDir + organize << std::endl;
          return false;
        }
    } else {
        outputPath = outputDir + output;
    }

    std::cout << "Recording... " << outputPath << std::endl;

    std::ostringstream ffmpeg_command;
    ffmpeg_command << "ffmpeg -loglevel error -fflags +discardcorrupt "
                   << "-headers \"X-Radiko-Authtoken: " << authtoken << "\" "
                   << "-i \"https://radiko.jp/v2/api/ts/playlist.m3u8?"
                   << "station_id=" << station_id
                   << "&ft=" << fromtime
                   << "&to=" << totime << "\" "
                   << "-acodec copy -vn -bsf:a aac_adtstoasc "
                   << "-metadata artist=\"" << personality << "\" "
                   << "-metadata album=\"" << organize << "\" "
                   << " -y \"" << outputPath << "\"";

    return std::system(ffmpeg_command.str().c_str()) == 0;
}

void logoutFromRadiko(const std::string& radiko_session) {
    if (radiko_session.empty()) return;

    std::ostringstream curl_command;
    curl_command << "curl --silent --request POST "
                 << "--data-urlencode \"radiko_session=" << radiko_session << "\" "
                 << "\"https://radiko.jp/v4/api/member/logout\"";
    std::system(curl_command.str().c_str());
}

