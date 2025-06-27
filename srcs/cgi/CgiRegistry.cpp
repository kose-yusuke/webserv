#include "CgiRegistry.hpp"
#include "CgiSession.hpp"
#include "Client.hpp"
#include "Logger.hpp"

CgiRegistry::CgiRegistry() {}

// fd closeはCgiSessionのインスタンスの責任になる
CgiRegistry::~CgiRegistry() {
  std::set<CgiSession *> unique_cgis;
  for (CgiIt it = fd_to_cgis_.begin(); it != fd_to_cgis_.end(); ++it) {
    unique_cgis.insert(it->second);
  }
  for (CgiSetIt sit = unique_cgis.begin(); sit != unique_cgis.end(); ++sit) {
    delete *sit;
  }
  fd_to_cgis_.clear();
}

void CgiRegistry::add(int fd, CgiSession *session) {
  if (has(fd)) {
    logfd(LOG_ERROR, "Duplicate cgi fd: ", fd);
    return;
  }
  fd_to_cgis_[fd] = session;
}

void CgiRegistry::remove(int fd) {
  CgiIt it = fd_to_cgis_.find(fd);
  if (it == fd_to_cgis_.end()) {
    return;
  }

  CgiSession *session = it->second;
  int stdin_fd = session->get_stdin_fd();
  int stdout_fd = session->get_stdout_fd();

  session->close_fd(fd);
  fd_to_cgis_.erase(it);
  if (!session->is_client_alive() &&
      fd_to_cgis_.count(stdin_fd) + fd_to_cgis_.count(stdout_fd) == 0) {
    delete session;
  }
}

CgiSession *CgiRegistry::get(int fd) const {
  ConstCgiIt it = fd_to_cgis_.find(fd);
  if (it == fd_to_cgis_.end()) {
    logfd(LOG_ERROR, "Failed to find cgi fd: ", fd);
    return NULL;
  }
  return it->second;
}

bool CgiRegistry::has(int fd) const {
  return fd_to_cgis_.find(fd) != fd_to_cgis_.end();
}

std::set<int> CgiRegistry::mark_timed_outs() {
  std::set<int> client_fds;
  std::set<CgiSession *> cgi_sessions;
  time_t now = time(NULL);
  for (CgiIt it = fd_to_cgis_.begin(); it != fd_to_cgis_.end(); ++it) {
    CgiSession *session = it->second;
    if (session->is_cgi_timeout(now)) {
      cgi_sessions.insert(session);
    }
  }
  for (CgiSetIt it = cgi_sessions.begin(); it != cgi_sessions.end(); ++it) {
    CgiSession *session = *it;
    session->on_cgi_timeout(); // fd close; pid SIGTERM; delete if Client dead
    if (session->is_client_alive()) {
      client_fds.insert(session->get_client_fd());
    }
  }
  return client_fds;
}

CgiRegistry &CgiRegistry::operator=(const CgiRegistry &other) {
  (void)other;
  return *this;
}
