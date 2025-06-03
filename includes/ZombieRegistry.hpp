#pragma once

#include <set>
#include <sys/types.h>

// CgiSessionのプロセス回収を、Clientができない時に用いる
class ZombieRegistry {
public:
  ZombieRegistry();
  ~ZombieRegistry();

  void track(pid_t pid);
  void reap_zombies();
  bool empty() const;
  void clear();
  size_t size() const;

private:
  std::set<pid_t> pending_zombies_;

  ZombieRegistry(const ZombieRegistry &other);
  ZombieRegistry &operator=(const ZombieRegistry &other);
};
