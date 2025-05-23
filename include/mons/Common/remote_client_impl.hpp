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
  // Listen for incoming heartbeats
  OnRecieve([&](const Message::Heartbeat& message)
  {
    lastBeat = std::chrono::system_clock::now();
    
    return false;
  });

  // Create a thread to send out heartbeats but don't hold it
  // It will keep running in the background
  new std::thread([&]()
  {
    Message::Heartbeat beat;
    while (true)
    {
      Send(beat);
      beat.HeartbeatData.beatCount++;
      sleep(5);
    }
  });
}

bool RemoteClient::IsResponsive()
{
  auto timeSinceLastBeat = std::chrono::system_clock::now() - lastBeat;
  double secondsSince = std::chrono::duration_cast<std::chrono::seconds>
      (timeSinceLastBeat).count();
  return (secondsSince < 30);
}

template <typename MessageType>
void RemoteClient::Send(MessageType& message)
{
  net->Send(message, id);
}

template <typename MessageType, typename ResponseType>
std::future<ResponseType> RemoteClient::SendAwaitable(MessageType& message)
{
  return net->SendAwaitable<MessageType, ResponseType>(message, id);
}

template <typename MessageType>
int RemoteClient::SendOpWait(MessageType& data, double timeout, int error)
{
  std::future<Message::OperationStatus> status =
      SendAwaitable<MessageType, Message::OperationStatus>(data);
  if (timeout > 0)
  {
    // Timeout logic
    if (status.wait_for(std::chrono::milliseconds((int)(timeout * 1000))) ==
        std::future_status::ready)
    {
      return status.get().OperationStatusData.status;
    }
    else
    {
      // Not ready (timed out)
      return error;
    }
  }
  else
  {
    // Wait infinitely
    return status.get().OperationStatusData.status;
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
