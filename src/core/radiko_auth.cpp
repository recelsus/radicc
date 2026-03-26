#include "core/radiko_auth.h"

#include "base64.hpp"
#include "core/radiko_http.h"

#include <iostream>

namespace radicc {
namespace {

constexpr char kAuthKeyValue[] = "bcd151073c03b352e1ef2fd66c32209da9ca0afa";

std::string generate_partial_key(int keyoffset, int keylength) {
  return base64::to_base64(std::string(kAuthKeyValue).substr(static_cast<size_t>(keyoffset), static_cast<size_t>(keylength)));
}

std::optional<std::string> find_json_string(const std::string& body, const std::string& key) {
  const std::string needle = "\"" + key + "\":\"";
  const std::size_t pos = body.find(needle);
  if (pos == std::string::npos) return std::nullopt;
  const std::size_t start = pos + needle.size();
  const std::size_t end = body.find('"', start);
  if (end == std::string::npos) return std::nullopt;
  return body.substr(start, end - start);
}

std::optional<bool> find_json_boolish(const std::string& body, const std::string& key) {
  const std::string needle = "\"" + key + "\":";
  const std::size_t pos = body.find(needle);
  if (pos == std::string::npos) return std::nullopt;
  const std::size_t start = pos + needle.size();
  if (body.compare(start, 1, "1") == 0) return true;
  if (body.compare(start, 1, "0") == 0) return false;
  if (body.compare(start, 4, "true") == 0) return true;
  if (body.compare(start, 5, "false") == 0) return false;
  return std::nullopt;
}

std::optional<std::string> extract_header_value(const std::string& headers, const std::string& name) {
  const std::string lower = name + ": ";
  const std::string upper = name.substr(0, 1) + name.substr(1);  // retained for call symmetry
  (void)upper;

  std::size_t line_start = 0;
  while (line_start < headers.size()) {
    const std::size_t line_end = headers.find('\n', line_start);
    const std::string line = trim_crlf(headers.substr(line_start, line_end == std::string::npos ? std::string::npos : line_end - line_start));

    std::string folded;
    folded.reserve(line.size());
    for (char ch : line) folded.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    if (folded.rfind(lower, 0) == 0) {
      return line.substr(name.size() + 2);
    }

    if (line_end == std::string::npos) break;
    line_start = line_end + 1;
  }
  return std::nullopt;
}

}  // namespace

std::optional<RadikoLoginSession> login_to_radiko(const std::string& mail, const std::string& password) {
  if (mail.empty() || password.empty()) {
    std::cerr << "Warning: No Radiko credentials provided. Skipping login.\n";
    return std::nullopt;
  }

  const auto body = curl_text({
      "curl", "--silent", "--request", "POST",
      "--data-urlencode", "mail=" + mail,
      "--data-urlencode", "pass=" + password,
      "https://radiko.jp/v4/api/member/login"});
  if (!body) {
    std::cerr << "Error: Failed to execute login command.\n";
    return std::nullopt;
  }

  auto session_id = find_json_string(*body, "radiko_session");
  if (!session_id || session_id->empty()) {
    std::cerr << "Login failed: Radiko session not found.\n";
    return std::nullopt;
  }

  RadikoLoginSession session;
  session.session_id = *session_id;
  session.is_areafree = find_json_boolish(*body, "areafree").value_or(false);
  return session;
}

std::optional<RadikoAuthState> authorize_radiko(const std::string& session_id) {
  const auto auth1_headers = curl_text({
      "curl", "--silent",
      "--header", "X-Radiko-App: pc_html5",
      "--header", "X-Radiko-App-Version: 0.0.1",
      "--header", "X-Radiko-Device: pc",
      "--header", "X-Radiko-User: dummy_user",
      "--dump-header", "-",
      "--output", "/dev/null",
      "https://radiko.jp/v2/api/auth1"});
  if (!auth1_headers) {
    std::cerr << "Error: Failed to execute auth1 command.\n";
    return std::nullopt;
  }

  auto authtoken = extract_header_value(*auth1_headers, "x-radiko-authtoken");
  auto keyoffset = extract_header_value(*auth1_headers, "x-radiko-keyoffset");
  auto keylength = extract_header_value(*auth1_headers, "x-radiko-keylength");
  if (!authtoken || !keyoffset || !keylength) {
    std::cerr << "Authorization failed: Required fields not found.\n";
    return std::nullopt;
  }

  const std::string partialkey = generate_partial_key(std::stoi(*keyoffset), std::stoi(*keylength));
  std::string auth2_url = "https://radiko.jp/v2/api/auth2";
  if (!session_id.empty()) auth2_url += "?radiko_session=" + session_id;

  const auto auth2_body = curl_text({
      "curl", "--silent",
      "--header", "X-Radiko-Device: pc",
      "--header", "X-Radiko-User: dummy_user",
      "--header", "X-Radiko-AuthToken: " + *authtoken,
      "--header", "X-Radiko-PartialKey: " + partialkey,
      auth2_url});
  if (!auth2_body) return std::nullopt;

  const std::string normalized = trim_crlf(*auth2_body);
  if (normalized.empty() || normalized == "OUT") {
    std::cerr << "Authorization failed: auth2 returned empty/OUT.\n";
    return std::nullopt;
  }

  const std::size_t comma = normalized.find(',');
  if (comma == std::string::npos || comma == 0) {
    std::cerr << "Authorization failed: area_id not found.\n";
    return std::nullopt;
  }

  RadikoAuthState state;
  state.authtoken = *authtoken;
  state.area_id = normalized.substr(0, comma);
  return state;
}

void logout_from_radiko(const std::string& radiko_session) {
  if (radiko_session.empty()) return;
  std::string out;
  run_command_capture({
      "curl", "--silent", "--request", "POST",
      "--data-urlencode", "radiko_session=" + radiko_session,
      "https://radiko.jp/v4/api/member/logout"}, out);
}

}  // namespace radicc
