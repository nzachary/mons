#ifndef MONS_COMMON_NETWORK_IMPL_HPP
#define MONS_COMMON_NETWORK_IMPL_HPP

#include "network.hpp"

#include <fstream>

#include "log.hpp"
#include "Message/message_types.hpp"

namespace mons {

Network& Network::Get(id_t id)
{
  auto instance = instances.find(id);
  if (instance == instances.end())
  {
    // Instance not created yet
    instances.emplace(id, id);
    return instances.at(id);
  }
  else
  {
    // Return instance
    return instance->second;
  }
}

Network::Network(id_t id) : id(id)
{
  ParseNetworkConfig();

  // Allow connections to local machine from any address
  if ((endpoints[id].address().is_v4()))
    endpoints[id].address(asio::ip::address_v4::any());
  else
    endpoints[id].address(asio::ip::address_v6::any());

  // Initalize sockets
  for (size_t remote = 0; remote < endpoints.size(); remote++)
  {
    // These sockets aren't connected, that is done by `RemoteClient`
    std::shared_ptr<Socket> socket = std::make_shared<Socket>
        (endpoints[id], endpoints[remote]);
    sockets.push_back(socket);
  }
}

const std::map<id_t, asio::ip::tcp::endpoint>& Network::GetEndpoints() const
{
  return endpoints;
}

template <typename MessageType>
void Network::Send(MessageType& message,
                   id_t machine)
{
  if (!sockets[machine]->IsConnected())
  {
    Log::Error("Cannot send - not connected");
    return;
  }
  // Set message sender/reciever
  message.BaseData.sender = id;
  message.BaseData.reciever = machine;
  MessageBuffer buffer = message.Message::Base::Serialize();
  
  // Send data
  sockets[machine]->Send(buffer);
}

template <typename MessageType, typename ResponseType>
std::optional<std::future<ResponseType>> Network
::SendAwaitable(MessageType& message,
                id_t machine,
                uint64_t messageId)
{
  if (!sockets[machine]->IsConnected())
  {
    Log::Error("Cannot send - not connected");
    return std::optional<std::future<ResponseType>>(std::nullopt);
  }
  // Set message ID if it isn't already set
  if (messageId == (uint64_t)-1)
    messageId = idCounter++;
  message.BaseData.id = messageId;

  // Launch waiter
  std::condition_variable outerCv;
  std::mutex outerMtx;
  bool outerCvPayload = false;
  std::future<ResponseType> waitable = std::async(std::launch::async,
      [this, messageId, machine, &outerCv, &outerMtx, &outerCvPayload]()
  {
    ResponseType response;
    std::condition_variable cv;
    std::mutex m;
    bool cvPayload = false;
    // Register an event to wait for a response
    RegisterEvent(machine, [&](const ResponseType& recieved)
    {
      if (recieved.BaseData.responseTo == messageId)
      {
        response = recieved;
        // Notify done
        std::unique_lock l(m);
        cvPayload = true;
        cv.notify_all();
    
        return true;
      }
    
      return false;
    });
    {
      // Notify event registered
      std::unique_lock outerLock(outerMtx);
      outerCvPayload = true;
      outerCv.notify_all();
    }
    // Wait for event to trigger
    std::unique_lock lock(m);
    cv.wait(lock, [&]{ return cvPayload; });
    return response;
  });

  // Wait for event to be registered
  std::unique_lock lock(outerMtx);
  outerCv.wait(lock, [&]{ return outerCvPayload; });
  // Send message
  Send(message, machine);

  return waitable;
}

#define REGISTER(Val) \
void Network \
::RegisterEvent(id_t from, std::function<bool(const Message::Val&)> callback) \
{ \
  allCallbacks##Val[from].push_back(std::move(callback)); \
}
MONS_REGISTER_MESSAGE_TYPES
#undef REGISTER

void Network::StartRecieve(id_t peer)
{
  Socket& socket = *sockets[peer];
  while (true)
  {
    if (!sockets[peer]->IsConnected())
    {
      Log::Error("Cannot recieve - not connected");
      return;
    }
    // Recieve raw data
    MessageBuffer buf(0);
    asio::error_code ec;
    socket.Recieve(buf);

    // Try to decode the header
    Message::Base::BaseDataStruct header;
    Message::Base::SerializeHeader(header, buf, false);
  
    // Create a polymorphic pointer using the messageType
    std::shared_ptr<Message::Base> message(nullptr);
    #define REGISTER(Val) case Message::MessageTypes::Val: \
        message = std::make_shared<Message::Val>(); break;
    switch (header.messageType)
    {
      MONS_REGISTER_MESSAGE_TYPES
    }
    #undef REGISTER
  
    // Write the header into message
    message->BaseData = std::move(header);

    // Deserialize the rest of the message
    size_t bytes = message->Deserialize(buf);
    if (bytes != buf.data.size())
    {
      Log::Error("Error while deserializing message");
      continue;
    }

    // Call the callbacks associated with the message
    #define REGISTER(Val) case Message::MessageTypes::Val: \
        PropagateMessage(*(Message::Val*)message.get()); break;
    switch (message->BaseData.messageType)
    {
      MONS_REGISTER_MESSAGE_TYPES
    }
    #undef REGISTER
  }
}

void Network::ParseNetworkConfig()
{
  std::ifstream config;
  config.open("network_config.txt");
  if (config.is_open())
  {
    // Read file line by line
    std::string line;
    size_t lineNum = 0;
    while (std::getline(config, line))
    {
      // Skip line if it starts with a '#' or is blank
      if (line[0] == '#' || line == "")
        continue;

      // Split line into parts seperated by semicolons
      std::vector<std::string> parts;
      size_t start = 0;
      size_t end = 0;
      while (end != line.npos)
      {
        end = line.find(';', start);
        parts.push_back(line.substr(start, end - start));
        start = end + 1;
      }

      // Verify line has correct number of parts
      if (parts.size() != 3)
        Log::FatalError("Line does not have 3 parts (ID, ip, port): \"" +
            line + "\"");
      
      // Parse parts of the line
      uint64_t endpointId;
      uint16_t port;
      asio::ip::address ip;
      try
      {
        endpointId = std::stoul(parts[0]);
      }
      catch (...)
      {
        Log::Error("Failed to parse ID: \"" + parts[0] + "\" at line " +
            std::to_string(lineNum));
      }
      try
      {
        port = std::stoi(parts[2]);
      }
      catch (...)
      {
        Log::Error("Failed to parse port: \"" + parts[2] + "\" at line " +
            std::to_string(lineNum));
      }
      asio::error_code ec;
      ip = asio::ip::make_address(parts[1], ec);

      // Add endpoint
      asio::ip::tcp::endpoint endpoint(ip, port);
      endpoints[endpointId] = asio::ip::tcp::endpoint(endpoint);
      lineNum++;
    }

    if (config.bad())
    {
      // IO error
      Log::FatalError("IO error while reading network config.");
    }
  }
  else
  {
    Log::FatalError("Failed to open network config.");
  }
}

#define Q(v) #v
#define QQ(v) Q(v)
#define REGISTER(Val) void Network::PropagateMessage(const Message::Val& message) \
{ \
  auto& cb = allCallbacks##Val[message.BaseData.sender]; \
  cb.erase(std::remove_if(cb.begin(), cb.end(), \
      [&](std::function<bool(const Message::Val&)>& callback) \
      { return callback(message); }), cb.end()); \
}
MONS_REGISTER_MESSAGE_TYPES
#undef REGISTER
#undef QQ
#undef Q

} // namespace mons

#endif
