#pragma once

#include <string>

namespace networking::messaging {

/**
 * Contains information about a connection to a remote endpoint or foreign host.
 */
struct ConnectionInfo {
  /** used as identifier for the connection */
  std::string hostname;
  /** unique and unambiguous id of the other node in the cluster */
  std::string endpoint_id;
};

} // namespace networking::messaging
