#include "core/radiko_http.h"

#include <sys/wait.h>
#include <unistd.h>

namespace radicc {

int run_command_capture(const std::vector<std::string>& args, std::string& output) {
  int pipefd[2];
  if (pipe(pipefd) != 0) return -1;

  pid_t pid = fork();
  if (pid == 0) {
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());
    _exit(127);
  }

  if (pid < 0) {
    close(pipefd[0]);
    close(pipefd[1]);
    return -1;
  }

  close(pipefd[1]);
  char buf[4096];
  ssize_t n = 0;
  while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
    output.append(buf, static_cast<size_t>(n));
  }
  close(pipefd[0]);

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) return -1;
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return -1;
}

std::optional<std::string> curl_text(const std::vector<std::string>& args) {
  std::string result;
  if (run_command_capture(args, result) != 0) return std::nullopt;
  return result;
}

std::optional<std::string> curl_get_text(const std::string& url) {
  return curl_text({"curl", "--silent", "--fail", url});
}

std::string trim_crlf(std::string value) {
  while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) {
    value.pop_back();
  }
  return value;
}

}  // namespace radicc
