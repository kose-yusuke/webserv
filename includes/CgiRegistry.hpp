#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <vector>

class CgiSession;

class CgiRegistry {
public:
  CgiRegistry();
  ~CgiRegistry();

  void add(int fd, CgiSession *session);
  void remove(int fd);
  CgiSession *get(int fd) const;
  bool has(int fd) const;

  std::set<int> mark_timed_outs();

private:
  std::map<int, CgiSession *> fd_to_cgis_;

  typedef std::map<int, CgiSession *>::iterator CgiIt;
  typedef std::map<int, CgiSession *>::const_iterator ConstCgiIt;
  typedef std::set<CgiSession *>::iterator CgiSetIt;
  typedef std::set<CgiSession *>::const_iterator ConstCgiSetIt;

  CgiRegistry(const CgiRegistry &other);
  CgiRegistry &operator=(const CgiRegistry &other);
};
