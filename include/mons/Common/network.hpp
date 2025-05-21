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

namespace mons {

class Network
{
public:
  typedef uint32_t id_t;
  static const id_t NO_ID = -1;

  // Get or create a network
  static Network& Get(id_t id);

  // Use `Get` instead
  Network(id_t id);
private:
  // Send some message using a socket.
  template <typename MessageType>
  void Send(MessageType& data,
            id_t machine);

  // Send some message using a socket and get a future containing a response message
  template <typename MessageType, typename ResponseType>
  std::future<ResponseType>
  SendAwaitable(MessageType& data,
                id_t machine);

  // `RegisterEvent` Registers a callback to run when a message is recieved
  // Register events this way instead of templates to allow passing lambda
  #define REGISTER(Val) \
  void RegisterEvent(id_t from, std::function<void(const Message::Val&)> callback);
  MONS_REGISTER_MESSAGE_TYPES
  #undef REGISTER

  // Start a thread to recieve and handle messages
  void StartReciever();
  // Log an asio error if it is set.
  void LogError(const asio::error_code& error);
  // Send some raw binary data to the specified machine. Assumes socket is open and connected
  void SendDataRaw(const MessageBuffer& data,
                   id_t machine);
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
  std::map<id_t, std::vector<std::function<void(const Message::Val&)>>> \
      allCallbacks##Val; \
  void PropagateMessage(const Message::Val& message);
  MONS_REGISTER_MESSAGE_TYPES
  #undef REGISTER

  // Single `Network` instance per ID
  inline static std::map<id_t, Network> instances;

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
