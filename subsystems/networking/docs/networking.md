# Networking and Node Connectivity

The networking subsystem is responsible for establishing and maintaining connections between nodes in the hive. It
currently supports persistent web-socket tunnels
for messaging, but the `IMessagingEndpoint` interface can be easily extended to support other protocols like ZeroMQ or
REST, depending on the underlying use case.

## Messaging

### Basic Message Structure

Hive abstracts any kind of information that can flow between nodes as
a protocol- and format-independent [Message](\ref hive::networking::messaging::Message). A message object consists of

* a unique identifier,
* a **type** that describes the message's content, and
* **attributes** that contain payload.

A [Message](\ref hive::networking::messaging::Message) is serialized and deserialized before sending. The currently used
standard
is [Multipart Form-Data](https://documentation.softwareag.com/webmethods/cloudstreams/wst10-5/10-5_CloudStreams_webhelp/index.html#page/cloudstreams-webhelp/to-custom_connector_15.html).
JSON is only used to serialize the message's metadata, like its type of unique id. Each attribute is wrapped into its
own message part and can even carry binary data.

> Attributes can contain binary data when sending images or files. JSON is very bad at this, because inserting raw
> binary data corrupts its
> encoding and syntax. One would need to Base64 encode it first, but this is very inefficient for quick message passing.
> Multipart Form-Data basically cuts a message into multiple parts (as its name implies), which can each have its own
> encoding and carry even larger binary objects.

Obviously there are more possible formats, like [Protobuf](https://protobuf.dev/), for this use case. However, they are
not yet implemented.

### Handling Message Types

A [IMessageEndpoint](\ref hive::networking::messaging::IMessageEndpoint) can be used to send and receive messages.
Incoming [Message](\ref hive::networking::messaging::Message) objects are dispatched to
registered [IMessageConsumer](\ref hive::networking::messaging::IMessageConsumer) handlers based on their type. Each
handler is
responsible for processing a specific message type.

> An example for this is found in the service infrastructure. It uses messaging to
> pass [ServiceRequest](\ref hive::services::ServiceRequest) and [ServiceResponse](\ref hive::services::ServiceResponse)
> messages (with types `service-request` and `service-response` respectively) between nodes. It registers
> the [RemoteServiceRequestConsumer](\ref hive::services::impl::RemoteServiceRequestConsumer)
> and [RemoteServiceResponseConsumer](\ref hive::services::impl::RemoteServiceResponseConsumer) implementations to
> handle
> these messages and process the service.

```cpp
using namespace hive::common::memory;
using namespace hive::networking::messaging;

Borrower<IMessageEndpoint> message_endpoint = BorrowMessageEndpoint();

// prints "world" when receiving a message of type "hello"
std::shared_ptr<IMessageConsumer> hello_consumer = std::make_shared<HelloMessageConsumer>();
message_endpoint->AddConsumer("hello", hello_consumer);
```

Incoming messages with types that do not have a registered handler are dropped.

## Implementations

Message-passing can be implemented in various ways. The current implementation uses web-sockets for persistent
connections. The `IMessagingEndpoint` interface can be extended to support other protocols like ZeroMQ or REST.

### Web-Socket Messaging

The [Web-Socket Protocol](https://en.wikipedia.org/wiki/WebSocket) is a communication protocol that provides full-duplex
communication channels over a single, persistent TCP connection. A single handshake is performed when connecting to
other nodes, not every time a message is sent. This makes it very efficient for passing messages quickly between nodes.

The [Boost::Beast](https://www.boost.org/doc/libs/1_86_0/libs/beast/doc/html/index.html) library provides web-socket
connections and is used for
the [BoostWebSocketEndpoint](\ref hive::networking::messaging::websockets::BoostWebSocketEndpoint)
implementation.