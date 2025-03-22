#pragma once

#include <map>
#include <string>
#include <vector>

// 汎用
typedef std::vector<std::string> StrVector;
typedef std::map<std::string, std::string> StrToStrMap;

// config
typedef std::map<std::string, std::vector<std::string> > ConfigMap;
typedef std::map<std::string, std::map<std::string, std::vector<std::string > > > LocationMap;
typedef std::vector<std::pair<ConfigMap, LocationMap> > ServerAndLocationConfigs;

// iterator
typedef ConfigMap::iterator ConfigIt;
typedef ConfigMap::const_iterator ConstConfigIt;
typedef LocationMap::iterator LocationIt;
typedef LocationMap::const_iterator ConstLocationIt;
