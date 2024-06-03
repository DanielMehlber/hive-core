#ifndef WEBSOCKETCONNECTIONINFO_H
#define WEBSOCKETCONNECTIONINFO_H

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

#endif // WEBSOCKETCONNECTIONINFO_H
