#include "ServerBuilder.hpp"
#include "Server.hpp"
#include "ServerRegistry.hpp"
#include <stdexcept>

void ServerBuilder::build(const ServerAndLocationConfigs &config_pairs,
                          ServerRegistry &registry) {
  if (config_pairs.empty()) {
    throw std::runtime_error("No valid server configurations found.");
  }
  for (size_t i = 0; i < config_pairs.size(); i++) {
    Server server(config_pairs[i].first, config_pairs[i].second);
    registry.add(server);
  }
}
