/*
 * Representation of the connected machines in the network
 */
#ifndef MONS_COMMON_NETWORK_HPP
#define MONS_COMMON_NETWORK_HPP

#include <asio.hpp>

#include <vector>
#include <memory> // std::shared_ptr
#include <mutex>
#include <map>
#include <optional>

#include "Message/message_types.hpp"
#include "socket.hpp"

namespace mons {

class Network
{
public:
  typedef uint32_t id_t;
  static const id_t NO_ID = -1;

  // Get or create a network
  static Network& Get(id_t id);

  // Internal constructor. Use `Get` instead
  Network(id_t id);

  // Returns a list of known endpoints
  const std::map<id_t, asio::ip::tcp::endpoint>& GetEndpoints() const;
private:
  // Send some message to another machine.
  template <typename MessageType>
  void Send(MessageType& data,
            id_t machine);

  // Send some message to another machine and get a future containing a response message
  // `messageId` can be set to override the sent message ID
  template <typename MessageType, typename ResponseType>
  std::optional<std::future<ResponseType>>
  SendAwaitable(MessageType& data,
                id_t machine,
                uint64_t messageId = -1);

  // `RegisterEvent` Registers a callback to run when a message is recieved
  // Return true from the callback if it should be deleted or false if it should remain
  #define REGISTER(Val) \
  void RegisterEvent(id_t from, std::function<bool(const Message::Val&)> callback);
  MONS_REGISTER_MESSAGE_TYPES
  #undef REGISTER

  // Recieve messages from a peer until the connection is broken
  void StartRecieve(id_t peer);
  // Parse network config from `network_config.txt`
  // Format is ID;IP;PORT
  void ParseNetworkConfig();

  // `allCallbacks[MessageType]` constains a list of all callbacks for MessageType
  // `PropagateMessage` will call all of the enabled callbacks for the message type
  #define REGISTER(Val) \
  std::map<id_t, std::vector<std::function<bool(const Message::Val&)>>> \
      allCallbacks##Val; \
  void PropagateMessage(const Message::Val& message);
  MONS_REGISTER_MESSAGE_TYPES
  #undef REGISTER

  // Single `Network` instance per ID
  inline static std::map<id_t, Network> instances;

  // Local mutex to avoid race conditions
  std::mutex mutex;

  // asio execution context
  asio::io_context context;

  // Sockets to remote endpoints
  std::vector<std::shared_ptr<Socket>> sockets;

  // The machine's network ID.
  // The server's ID is 0 and the clients have an ID
  // counting up in the order of connection.
  id_t id = NO_ID;
  // Network endpoints, index is equal to endpoint machine's ID.
  std::map<id_t, asio::ip::tcp::endpoint> endpoints;
  // Message ID counter
  uint64_t idCounter = 1;

  friend class RemoteClient;
};

} // namespace mons

#include "network_impl.hpp"

#endif
