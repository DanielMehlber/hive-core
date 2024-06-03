#pragma once

#include <string>

namespace networking::messaging {

/**
 * Contains information about a connection to a remote endpoint or foreign host.
 */
struct ConnectionInfo {
  /** used as identifier for the connection */
  std::string hostname;
};

} // namespace networking::messaging
