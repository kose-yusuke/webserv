#include "ZombieRegistry.hpp"
#include <iostream>
#include <sys/wait.h>

ZombieRegistry::ZombieRegistry() {}

ZombieRegistry::~ZombieRegistry() {}

void ZombieRegistry::track(pid_t pid) { pending_zombies_.insert(pid); }

void ZombieRegistry::reap_zombies() {
  std::set<pid_t>::iterator it = pending_zombies_.begin();
  while (it != pending_zombies_.end()) {
    int status;
    pid_t result = waitpid(*it, &status, WNOHANG);
    if (*it == result) {
      it = pending_zombies_.erase(it);
    } else {
      ++it;
    }
  }
}

bool ZombieRegistry::empty() const { return pending_zombies_.empty(); }

void ZombieRegistry::clear() { pending_zombies_.clear(); }

size_t ZombieRegistry::size() const { return pending_zombies_.size(); }

ZombieRegistry &ZombieRegistry::operator=(const ZombieRegistry &other) {
  (void)other;
  return *this;
}
