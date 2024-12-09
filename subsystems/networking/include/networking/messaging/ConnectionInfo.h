#pragma once

#include <string>

namespace hive::networking::messaging {

/**
 * Contains information about a connection to a remote endpoint or foreign host.
 */
struct ConnectionInfo {
  /** used as identifier for the connection */
  std::string remote_url;
  /** unique and unambiguous id of the other node in the hive */
  std::string remote_endpoint_id;
  /** IP-address and port of the remote endpoint */
  std::string remote_host_name;
  /** IP-address and port of the local endpoint */
  std::string local_host_name;
};

} // namespace hive::networking::messaging
