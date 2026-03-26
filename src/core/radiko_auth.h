#pragma once

#include <optional>
#include <string>

namespace radicc {

struct RadikoLoginSession {
  std::string session_id;
  bool is_areafree = false;
};

struct RadikoAuthState {
  std::string authtoken;
  std::string area_id;
};

std::optional<RadikoLoginSession> login_to_radiko(const std::string& mail, const std::string& password);
std::optional<RadikoAuthState> authorize_radiko(const std::string& session_id);
void logout_from_radiko(const std::string& radiko_session);

}  // namespace radicc
