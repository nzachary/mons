/*
 * Representation of a connected client
 */
#ifndef MONS_COMMON_REMOTE_CLIENT_HPP
#define MONS_COMMON_REMOTE_CLIENT_HPP

#include "network.hpp"

namespace mons {

class RemoteClient {
public:
  // Get or create a client on network with specified remote ID
  static RemoteClient& Get(Network& network, id_t id);

  // Use `Get` instead
  RemoteClient(Network& network, id_t id);

  bool IsResponsive();

  // Send a message to client
  template <typename MessageType>
  void Send(MessageType& message);

  // Send a message to client and get a future containing a response message
  template <typename MessageType, typename ResponseType>
  std::future<ResponseType>
  SendAwaitable(MessageType& data);

  // Called when message type is recieved from client
  // Register this way instead of templates to allow passing lambda
  #define REGISTER(Val) \
  void OnRecieve(std::function<void(const Message::Val&)> callback);
  MONS_REGISTER_MESSAGE_TYPES
  #undef REGISTER
private:
  // All instances
  // The first ID is the network host ID
  // Second ID is client ID
  inline static std::map<std::tuple<id_t, id_t>, RemoteClient> instances;

  // ID of client
  id_t id;

  // Network this client is on
  Network* net;

  // Time point of last recieved heartbeat
  std::chrono::time_point<std::chrono::system_clock> lastBeat =
      std::chrono::system_clock::now();
};

} // namespace mons

#include "remote_client_impl.hpp"

#endif
