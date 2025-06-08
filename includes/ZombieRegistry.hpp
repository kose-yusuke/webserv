#pragma once

#include <map>
#include <sys/types.h>

// CgiSessionのプロセス回収を、Clientができない時に用いる
class ZombieRegistry {
public:
  ZombieRegistry();
  ~ZombieRegistry();

  void track(pid_t pid);
  void reap_zombies();
  void manage_zombies();
  bool empty() const;
  void clear();
  size_t size() const;

private:
  std::map<pid_t, time_t> pending_zombies_;

  typedef std::map<pid_t, time_t>::iterator ZombiesIt;

  ZombieRegistry(const ZombieRegistry &other);
  ZombieRegistry &operator=(const ZombieRegistry &other);
};
