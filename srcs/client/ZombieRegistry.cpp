#include "ZombieRegistry.hpp"
#include "Logger.hpp"
#include <iostream>
#include <signal.h>
#include <sys/wait.h>

static const int k_grace_seconds = 10; // TODO: 秒数は要検討

ZombieRegistry::ZombieRegistry() {}

ZombieRegistry::~ZombieRegistry() {}

void ZombieRegistry::track(pid_t pid) {

  if (pid == -1) {
    return;
  }
  pid_t result = waitpid(pid, NULL, WNOHANG);

  if (result == pid) {
    logfd(LOG_DEBUG, "Immediately reaped zombie pid=", pid);
    return;
  }
  if (result == -1 && errno == ECHILD) {
    logfd(LOG_DEBUG, "Tried to track a PID that's already reaped: ", pid);
    return;
  }
  pending_zombies_[pid] = time(NULL);
}

void ZombieRegistry::reap_zombies() {

  pid_t result;

  while ((result = waitpid(-1, NULL, WNOHANG)) > 0) {
    logfd(LOG_DEBUG, "reap child zombie of pid=", result);
    pending_zombies_.erase(result);
  }
}

void ZombieRegistry::manage_zombies() {

  time_t now = time(NULL);

  for (ZombiesIt it = pending_zombies_.begin(); it != pending_zombies_.end();) {

    pid_t result = waitpid(it->first, NULL, WNOHANG);
    if (result == it->first || (result == -1 && errno == ECHILD)) {
      it = pending_zombies_.erase(it);
      continue;
    }

    if (now - it->second > k_grace_seconds) {
      kill(it->first, SIGKILL);
    }
    ++it;
  }
}

bool ZombieRegistry::empty() const { return pending_zombies_.empty(); }

void ZombieRegistry::clear() { pending_zombies_.clear(); }

size_t ZombieRegistry::size() const { return pending_zombies_.size(); }

ZombieRegistry &ZombieRegistry::operator=(const ZombieRegistry &other) {
  (void)other;
  return *this;
}
