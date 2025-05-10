#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

// 汎用
typedef std::vector<std::string> StrVector;
typedef std::map<std::string, std::string> StrToStrMap;

// config
typedef std::map<std::string, std::vector<std::string> > ConfigMap;
typedef std::map<std::string, std::map<std::string, std::vector<std::string> > >
    LocationMap;
typedef std::vector<std::pair<ConfigMap, LocationMap> > ServerAndLocationConfigs;

// iterator
typedef ConfigMap::iterator ConfigIt;
typedef ConfigMap::const_iterator ConstConfigIt;
typedef LocationMap::iterator LocationIt;
typedef LocationMap::const_iterator ConstLocationIt;

typedef StrToStrMap::iterator StrToStrMapIt;
typedef StrToStrMap::const_iterator ConstStrToStrMapIt;

typedef std::pair<std::string, std::string> ListenPair;
// HeaderMap

struct CaseInsensitiveLess {
  bool operator()(const std::string &a, const std::string &b) const {
    std::string lower_a = a;
    std::string lower_b = b;
    std::transform(a.begin(), a.end(), lower_a.begin(), ::tolower);
    std::transform(b.begin(), b.end(), lower_b.begin(), ::tolower);
    return lower_a < lower_b;
  }
};

typedef std::map<std::string, std::vector<std::string>, CaseInsensitiveLess>
    HeaderMap;

typedef HeaderMap::iterator HeaderMapIt;
typedef HeaderMap::const_iterator ConstHeaderMapIt;
