#include "app/common.h"
#include "app/command_options.h"
#include "service/record_service.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

namespace radicc {
namespace {

struct DownloadEntry {
  std::string absolute_path;
  std::string filename;
};

std::mutex g_downloads_mutex;
std::string g_current_job_id;
std::optional<DownloadEntry> g_current_download;

std::string url_decode(const std::string& value) {
  std::string decoded;
  decoded.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '%' && i + 2 < value.size()) {
      const std::string hex = value.substr(i + 1, 2);
      decoded.push_back(static_cast<char>(std::strtol(hex.c_str(), nullptr, 16)));
      i += 2;
    } else if (value[i] == '+') {
      decoded.push_back(' ');
    } else {
      decoded.push_back(value[i]);
    }
  }
  return decoded;
}

std::unordered_map<std::string, std::string> parse_query(const std::string& query) {
  std::unordered_map<std::string, std::string> values;
  std::size_t start = 0;
  while (start < query.size()) {
    const std::size_t amp = query.find('&', start);
    const std::string part = query.substr(start, amp == std::string::npos ? std::string::npos : amp - start);
    const std::size_t eq = part.find('=');
    const std::string key = url_decode(part.substr(0, eq));
    const std::string value = eq == std::string::npos ? std::string() : url_decode(part.substr(eq + 1));
    if (!key.empty()) values[key] = value;
    if (amp == std::string::npos) break;
    start = amp + 1;
  }
  return values;
}

std::optional<std::string> extract_json_string(const std::string& body, const std::string& key) {
  const std::regex pattern("\"" + key + R"(\"\s*:\s*\"([^\"]*)\")");
  std::smatch match;
  if (!std::regex_search(body, match, pattern) || match.size() < 2) return std::nullopt;
  return match[1].str();
}

std::optional<int> extract_json_int(const std::string& body, const std::string& key) {
  const std::regex pattern("\"" + key + R"(\"\s*:\s*(-?\d+))");
  std::smatch match;
  if (!std::regex_search(body, match, pattern) || match.size() < 2) return std::nullopt;
  return std::stoi(match[1].str());
}

std::string make_job_id() {
  static constexpr char kHex[] = "0123456789abcdef";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0, 15);
  std::string id;
  id.reserve(16);
  for (int i = 0; i < 16; ++i) id.push_back(kHex[dist(gen)]);
  return id;
}

void send_all(int fd, const std::string& data) {
  std::size_t sent = 0;
  while (sent < data.size()) {
    const ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, 0);
    if (n <= 0) return;
    sent += static_cast<std::size_t>(n);
  }
}

void send_response(int fd, int status, const std::string& status_text, const std::string& content_type, const std::string& body, const std::string& extra_headers = {}) {
  std::ostringstream response;
  response << "HTTP/1.1 " << status << ' ' << status_text << "\r\n"
           << "Content-Type: " << content_type << "\r\n"
           << "Content-Length: " << body.size() << "\r\n"
           << "Connection: close\r\n";
  if (!extra_headers.empty()) response << extra_headers;
  response << "\r\n" << body;
  send_all(fd, response.str());
}

void send_json(int fd, int status, const std::string& status_text, const std::string& json) {
  send_response(fd, status, status_text, "application/json; charset=utf-8", json);
}

void send_file(int fd, const DownloadEntry& entry) {
  std::ifstream file(entry.absolute_path, std::ios::binary);
  if (!file.is_open()) {
    send_json(fd, 404, "Not Found", "{\"error\":\"file not found\"}");
    return;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  const std::string body = buffer.str();
  const std::string headers = "Content-Disposition: attachment; filename=\"" + entry.filename + "\"\r\n";
  send_response(fd, 200, "OK", "audio/mp4", body, headers);
}

std::string build_help_json() {
  return "{"
         "\"endpoints\":["
         "\"GET /health\","
         "\"POST /record (application/json: {\\\"url\\\":\\\"https://radiko.jp/#!/ts/JORF/20260322003000\\\",\\\"date_offset\\\":1})\","
         "\"GET /download/{job_id}\""
         "]"
         "}";
}

void handle_record(int fd, const std::string& body) {
  const auto url = extract_json_string(body, "url");
  if (!url || url->empty()) {
    send_json(fd, 400, "Bad Request", "{\"error\":\"url is required\"}");
    return;
  }

  CommandOptions options;
  options.url = *url;

  const auto date_offset = extract_json_int(body, "date_offset");
  if (date_offset.has_value()) {
    try {
      options.date_offset = *date_offset;
      if (options.date_offset < 0) throw std::invalid_argument("negative");
      options.date_offset_set = true;
    } catch (const std::exception&) {
      send_json(fd, 400, "Bad Request", "{\"error\":\"date_offset must be a non-negative integer\"}");
      return;
    }
  }

  try {
    const auto result = execute_record_request(options);
    const std::string job_id = make_job_id();
    {
      std::lock_guard<std::mutex> lock(g_downloads_mutex);
      g_current_job_id = job_id;
      g_current_download = DownloadEntry{result.paths.absolute_path, result.paths.filename};
    }

    const std::string json = "{"
        "\"status\":\"done\","
        "\"job_id\":\"" + json_escape(job_id) + "\","
        "\"download_url\":\"/download/" + json_escape(job_id) + "\","
        "\"filepath\":\"" + json_escape(result.paths.absolute_path) + "\","
        "\"output_file\":\"" + json_escape(result.paths.filename) + "\","
        "\"title\":\"" + json_escape(result.resolved.title) + "\","
        "\"start_time\":\"" + json_escape(result.start_time) + "\","
        "\"end_time\":\"" + json_escape(result.end_time) + "\""
        "}";
    send_json(fd, 200, "OK", json);
  } catch (const RadiccError& error) {
    send_json(fd, 400, "Bad Request", std::string("{\"error\":\"") + json_escape(error.what()) + "\"}");
  } catch (const std::exception& error) {
    send_json(fd, 500, "Internal Server Error", std::string("{\"error\":\"") + json_escape(error.what()) + "\"}");
  }
}

void handle_client(int fd) {
  char buffer[8192];
  const ssize_t n = ::recv(fd, buffer, sizeof(buffer) - 1, 0);
  if (n <= 0) return;
  buffer[n] = '\0';

  const std::string raw_request(buffer, static_cast<std::size_t>(n));
  const std::size_t header_end = raw_request.find("\r\n\r\n");
  const std::string headers = header_end == std::string::npos ? raw_request : raw_request.substr(0, header_end);
  const std::string body = header_end == std::string::npos ? std::string() : raw_request.substr(header_end + 4);

  std::istringstream request(headers);
  std::string method;
  std::string target;
  std::string version;
  request >> method >> target >> version;

  const std::size_t query_pos = target.find('?');
  const std::string path = target.substr(0, query_pos);

  if (method == "GET" && (path == "/" || path == "/help")) {
    send_json(fd, 200, "OK", build_help_json());
    return;
  }
  if (method == "GET" && path == "/health") {
    send_json(fd, 200, "OK", "{\"status\":\"ok\"}");
    return;
  }
  if (path == "/record") {
    if (method != "POST") {
      send_json(fd, 405, "Method Not Allowed", "{\"error\":\"POST only\"}");
      return;
    }
    if (headers.find("Content-Type: application/json") == std::string::npos
        && headers.find("Content-Type: application/json;") == std::string::npos) {
      send_json(fd, 415, "Unsupported Media Type", "{\"error\":\"application/json is required\"}");
      return;
    }
    handle_record(fd, body);
    return;
  }
  if (method == "GET" && path.rfind("/download/", 0) == 0) {
    const std::string job_id = path.substr(std::string("/download/").size());
    std::lock_guard<std::mutex> lock(g_downloads_mutex);
    if (!g_current_download || job_id != g_current_job_id) {
      send_json(fd, 404, "Not Found", "{\"error\":\"unknown job id\"}");
      return;
    }
    send_file(fd, *g_current_download);
    return;
  }

  if (method != "GET" && method != "POST") {
    send_json(fd, 405, "Method Not Allowed", "{\"error\":\"GET and POST only\"}");
    return;
  }

  send_json(fd, 404, "Not Found", "{\"error\":\"not found\"}");
}

}  // namespace
}  // namespace radicc

int main(int argc, char* argv[]) {
  using namespace radicc;

  std::string bind_host = "127.0.0.1";
  int bind_port = 8080;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if ((arg == "--host" || arg == "-h") && i + 1 < argc) {
      bind_host = argv[++i];
    } else if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
      bind_port = std::stoi(argv[++i]);
    } else if (arg == "--help") {
      std::cout << "Usage: radicc-server [--host 127.0.0.1] [--port 8080]\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << arg << std::endl;
      return 1;
    }
  }

  const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "socket() failed: " << std::strerror(errno) << std::endl;
    return 1;
  }

  int yes = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(bind_port));
  if (::inet_pton(AF_INET, bind_host.c_str(), &addr.sin_addr) != 1) {
    std::cerr << "Invalid bind host: " << bind_host << std::endl;
    ::close(server_fd);
    return 1;
  }

  if (::bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "bind() failed: " << std::strerror(errno) << std::endl;
    ::close(server_fd);
    return 1;
  }
  if (::listen(server_fd, 16) < 0) {
    std::cerr << "listen() failed: " << std::strerror(errno) << std::endl;
    ::close(server_fd);
    return 1;
  }

  std::cout << "radicc-server listening on http://" << bind_host << ':' << bind_port << std::endl;
  while (true) {
    const int client_fd = ::accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) continue;
    std::thread([client_fd]() {
      handle_client(client_fd);
      ::close(client_fd);
    }).detach();
  }
}
