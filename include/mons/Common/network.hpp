/*
 * Representation of the connected machines in the network
 */
#ifndef MONS_COMMON_NETWORK_HPP
#define MONS_COMMON_NETWORK_HPP

#include <asio.hpp>

#include <vector>
#include <memory> // std::shared_ptr

#include "Message/message_types.hpp"

namespace mons {

class Network
{
public:
  typedef uint32_t id_t;
  static const id_t NO_ID = -1;

  // Set local machine ID; expected to match ID in network config file
  Network(id_t id);
  // Send some message to the specified machine.
  template <typename MessageType>
  void Send(MessageType data,
            id_t machine);
  
  // Send some message to the specified machine and get a future that contains a response
  template <typename MessageType, typename ResponseType>
  std::future<ResponseType>
  SendAwaitable(MessageType data,
                id_t machine);

  // `RegisterEvent` Registers a callback to run when a message is recieved
  #define REGISTER(Val) \
  void RegisterEvent(std::function<void(const Message::Val&)> callback);
  MONS_REGISTER_MESSAGE_TYPES
  #undef REGISTER
private:
  // Blocking call to recieve a message.
  std::shared_ptr<Message::Base> Recieve();
  // Log an asio error if it is set.
  void LogError(const asio::error_code& error);
  // Send some raw binary data to the specified machine.
  void SendDataRaw(const std::vector<char>& data, id_t machine);
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
  std::vector<std::function<void(const Message::Val&)>> allCallbacks##Val; \
  void PropagateMessage(const Message::Val& message);
  MONS_REGISTER_MESSAGE_TYPES
  #undef REGISTER

  // Thread to run the event handler listener on
  std::thread listenerThread;

  // Thread to send heart beats from
  std::thread heartThread;

  // The machine's network ID.
  // The server's ID is 0 and the clients have an ID
  // counting up in the order of connection.
  id_t id = NO_ID;
  // Network endpoints, index is equal to endpoint machine's ID.
  std::vector<asio::ip::tcp::endpoint> endpoints;
  // Time of last heartbeat from machine
  std::vector<std::chrono::time_point<std::chrono::system_clock>> lastHeartbeat;
  // List of machines we want to let know that we are alive
  std::vector<id_t> connected;
  // Message ID counter
  uint64_t idCounter = 1;
};

} // namespace mons

#include "network_impl.hpp"

#endif
