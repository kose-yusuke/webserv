#pragma once

#include "types.hpp"

class ServerRegistry;

class ServerBuilder {
public:
  static void build(const ServerAndLocationConfigs &config_pairs,
                    ServerRegistry &registry);
};
