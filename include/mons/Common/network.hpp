/*
 * Representation of the connected machines in the network
 */
#ifndef MONS_COMMON_NETWORK_HPP
#define MONS_COMMON_NETWORK_HPP

#include <asio.hpp>

#include <vector>
#include <memory> // std::shared_ptr
#include <map>

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
private:
  // Send some message using a socket.
  template <typename MessageType>
  void Send(MessageType& data,
            id_t machine);

  // Send some message using a socket and get a future containing a response message
  template <typename MessageType, typename ResponseType>
  std::optional<std::future<ResponseType>>
  SendAwaitable(MessageType& data,
                id_t machine);

  // `RegisterEvent` Registers a callback to run when a message is recieved
  // Register events this way instead of templates to allow passing lambda
  // Return true from the callback if it should be deleted
  #define REGISTER(Val) \
  void RegisterEvent(id_t from, std::function<bool(const Message::Val&)> callback);
  MONS_REGISTER_MESSAGE_TYPES
  #undef REGISTER

  // Recieve messages from a peer until the connection is broken
  void StartRecieve(id_t peer);
  // Parse network config from `network_config.txt`
  // Format is [ID];[IP];[PORT]
  void ParseNetworkConfig();
  // Add a machine with an endpoint and ID
  void AddMachine(const asio::ip::tcp::endpoint& endpoint, id_t machine);
  // Add a machine to `connected` if it isn't already being tracked
  void AddUniqueConnected(id_t machine);

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

  // asio execution context
  asio::io_context context;

  // Sockets to remote endpoints
  std::vector<std::shared_ptr<Socket>> sockets;

  // The machine's network ID.
  // The server's ID is 0 and the clients have an ID
  // counting up in the order of connection.
  id_t id = NO_ID;
  // Network endpoints, index is equal to endpoint machine's ID.
  std::vector<asio::ip::tcp::endpoint> endpoints;
  // Message ID counter
  uint64_t idCounter = 1;

  friend class RemoteClient;
};

} // namespace mons

#include "network_impl.hpp"

#endif
