# Runtime Configuration

The following configuration properties can be set:

## Job System

* `jobs.concurrency` the number of threads to use for the job system. Defaults to the number of available CPU cores.

## Networking Subsystem

General configuration of the networking manager:

* `net.node.id` the unique identifier of the node in the network (otherwise randomly generated).
* `net.startup` whether to invoke the primary endpoint at startup. If `false`, networking is disabled until an endpoint
  is initialized manually.

The primary and default implementation for the messaging endpoint is currently web-socket based. Websockets can be
configured using:

* `net.websocket.port` port to bind the connection listener to.
* `net.websocket.address` address to bind the connection listener to.
* `net.websocket.tls` whether to use TLS for the primary messaging endpoint.
* `net.websocket.threads` the number of threads to use for processing async events.