#ifndef MONS_COMMON_REMOTE_CLIENT_IMPL_HPP
#define MONS_COMMON_REMOTE_CLIENT_IMPL_HPP

#include "remote_client.hpp"

namespace mons {

RemoteClient& RemoteClient::Get(Network& network, id_t id)
{
  auto key = std::make_tuple(network.id, id);
  auto instance = instances.find(key);
  if (instance == instances.end())
  {
    // Instance not created yet
    instances.emplace(std::piecewise_construct,
        key, std::forward_as_tuple(network, id));
    return instances.at(key);
  }
  else
  {
    // Return instance
    return instance->second;
  }
}

RemoteClient::RemoteClient(Network& network, id_t id) : id(id), net(&network)
{
  Connect();
}

bool RemoteClient::Connect()
{
  // Server sends out connection requests, clients accept them
  bool isClient = (net->id != 0);
  bool success = false;
  if (isClient)
    success = net->sockets[this->id]->Listen();
  else
    success = net->sockets[this->id]->Connect();
  
  if (success)
  {
    // Start recieving messages from the client
    std::thread reciever([this]()
    {
      net->StartRecieve(this->id);
    });
    reciever.detach();

    // Send configuration information
    Message::ConnectInfo message;
    message.BaseData.responseTo = 0;
    auto returnInfo = SendAwaitable<Message::ConnectInfo,
        Message::ConnectInfo>(message, 0);
    // Check if config matches
    // See `config_hash.hpp`
    if (!returnInfo.has_value())
    {
      Log::Error("Error sending configuration info");
      return false;
    }
    else
    {
      if (returnInfo->wait_for(std::chrono::seconds(5)) !=
          std::future_status::ready)
      {
        Log::Error("Configuration timed out");
        return false;
      }
      else
      {
        if (returnInfo->get().ConnectInfoData.config == CONFIG_HASH)
          return true; // Config matches - connection is good
        else
        {
          Log::Error("Configuration mismatch");
          return false;
        }
      }
    }
  }
  return false;
}

bool RemoteClient::IsConnected()
{
  return net->sockets[id]->IsConnected();
}

template <typename MessageType>
void RemoteClient::Send(SafeRef<MessageType>& message)
{
  auto lock = message.Lock();
  return Send(message.Value(), id);
}

template <typename MessageType>
void RemoteClient::Send(MessageType& message)
{
  net->Send(message, id);
}

template <typename MessageType, typename ResponseType>
std::optional<std::future<ResponseType>> RemoteClient
::SendAwaitable(SafeRef<MessageType>& message, uint64_t messageId)
{
  auto lock = message.Lock();
  return SendAwaitable(message.Value(), messageId);
}

template <typename MessageType, typename ResponseType>
std::optional<std::future<ResponseType>> RemoteClient
::SendAwaitable(MessageType& message, uint64_t messageId)
{
  return net->SendAwaitable<MessageType, ResponseType>(message, id, messageId);
}

template <typename MessageType>
int RemoteClient::SendOpWait(SafeRef<MessageType>& data, double timeout)
{
  auto lock = data.Lock();
  return SendOpWait(data.Value(), timeout);
}

template <typename MessageType>
int RemoteClient::SendOpWait(MessageType& data, double timeout)
{
  std::optional<std::future<Message::OperationStatus>> status =
      SendAwaitable<MessageType, Message::OperationStatus>(data);
  if (!status.has_value())
    return 1;
  if (timeout > 0)
  {
    // Timeout logic
    if (status.value().wait_for(std::chrono::milliseconds((int)(timeout * 1000))) ==
        std::future_status::ready)
    {
      return status.value().get().OperationStatusData.status;
    }
    else
    {
      // Not ready (timed out)
      return 2;
    }
  }
  else
  {
    // Wait infinitely
    return status.value().get().OperationStatusData.status;
  }
}

#define REGISTER(Val) \
void RemoteClient::OnRecieve(std::function<bool(const Message::Val&)> callback) \
{ \
  net->RegisterEvent(id, std::move(callback)); \
}
MONS_REGISTER_MESSAGE_TYPES
#undef REGISTER

} // namespace mons

#endif
