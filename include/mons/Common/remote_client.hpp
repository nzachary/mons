/*
 * Representation of a connected client
 */
#ifndef MONS_COMMON_REMOTE_CLIENT_HPP
#define MONS_COMMON_REMOTE_CLIENT_HPP

#include "network.hpp"
#include "safe_ref.hpp"

namespace mons {

class RemoteClient {
public:
  // Get or create a client on network with specified remote ID
  static RemoteClient& Get(Network& network, id_t id);

  // Get the ID of the remote client
  id_t GetId() { return id; };

  // Internal constructor. Use `Get` instead
  RemoteClient(Network& network, id_t id);

  // Connect to the machine this is associated with
  bool Connect();

  // Checks if the connection is up
  bool IsConnected();

  // Send a message to client
  // Thread safe variant
  template <typename MessageType>
  void Send(SafeRef<MessageType>& message);

  // Send a message to client
  template <typename MessageType>
  void Send(MessageType& message);

  // Send a message to client and get a future containing a response message
  // Thread safe variant
  template <typename MessageType, typename ResponseType>
  std::optional<std::future<ResponseType>>
  SendAwaitable(SafeRef<MessageType>& data, uint64_t messageId = -1);

  // Send a message to client and get a future containing a response message
  template <typename MessageType, typename ResponseType>
  std::optional<std::future<ResponseType>>
  SendAwaitable(MessageType& data, uint64_t messageId = -1);

  // Send a message and wait for a status to be returned
  // If `timeout` is > 0, the function will time out after that many seconds
  // Thread safe variant
  template <typename MessageType>
  int SendOpWait(SafeRef<MessageType>& data, double timeout = 15);

  // Send a message and wait for a status to be returned
  // If `timeout` is > 0, the function will time out after that many seconds
  template <typename MessageType>
  int SendOpWait(MessageType& data, double timeout = 15);

  // Called when message type is recieved from client
  // Register this way instead of templates to allow passing lambda
  #define REGISTER(Val) \
  void OnRecieve(std::function<bool(const Message::Val&)> callback);
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
};

} // namespace mons

#include "remote_client_impl.hpp"

#endif
