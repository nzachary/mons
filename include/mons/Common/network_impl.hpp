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

  // Start listener thread
  StartReciever();
}

template <typename MessageType>
void Network::Send(MessageType& message,
                   id_t machine)
{
  // Set message sender/reciever
  message.BaseData.sender = id;
  message.BaseData.reciever = machine;
  // Set message ID
  message.BaseData.id = idCounter++;
  MessageBuffer buffer = message.Message::Base::Serialize();
  // Send message
  SendDataRaw(buffer, machine);
}

template <typename MessageType, typename ResponseType>
std::future<ResponseType> Network
::SendAwaitable(MessageType& message,
                id_t machine)
{
  Send(message, machine);
  // ID is set by Send()
  uint64_t messageId = message->BaseData.id;
  auto waitable = std::async(std::launch::async, [&]()
  {
    ResponseType response;
    RegisterEvent(machine, [&](const ResponseType& recieved)
    {
      if (recieved.BaseData.responseTo == messageId)
        response = std::move(recieved);
    });
    return response;
  });
  return waitable.future();
}

#define REGISTER(Val) \
void Network \
::RegisterEvent(id_t from, std::function<void(const Message::Val&)> callback) \
{ \
  allCallbacks##Val[from].push_back(std::move(callback)); \
}
MONS_REGISTER_MESSAGE_TYPES
#undef REGISTER

void Network::StartReciever()
{
  // Create a new thread but don't hold onto it
  // It will just keep running in the background
  new std::thread([&]()
  {
    while (true)
    {
      // Accept incoming connection
      asio::io_context context;
      asio::ip::tcp::acceptor acceptor(context, endpoints[id]);
      asio::ip::tcp::socket socket(context);
      asio::error_code ec;
      acceptor.accept(socket, ec);
      LogError(ec);

      // Recieve header
      Message::Base::BaseDataStruct header;
      MessageBuffer buffer(sizeof(header));
      socket.read_some(asio::buffer(buffer.data), ec);
      LogError(ec);

      // Try to decode the header
      Message::Base::SerializeHeader(header, buffer, false);

      // Check message API version
      if (header.apiVersion != MONS_VERSION_NUMBER) {
        Log::Error("Recieved message with version " +
            std::to_string(header.apiVersion) +
            " but expected " + std::to_string(MONS_VERSION_NUMBER));
        continue;
      }

      // Create a shared_ptr with the correct underlying class
      std::shared_ptr<Message::Base> message(nullptr);
      #define REGISTER(Val) case Message::MessageTypes::Val: \
          message = std::make_shared<Message::Val>(); break;
      switch (header.messageType)
      {
        MONS_REGISTER_MESSAGE_TYPES
      }
      #undef REGISTER

      // Resize buffer to accept rest of message
      buffer.data.resize(header.messageNumBytes - sizeof(header));
      buffer.deserializePtr = 0;
      // Read the rest of the message
      size_t totalRead = 0;
      while (totalRead < buffer.data.size())
      {
        auto asioBuffer = asio::buffer(buffer.data.data() + totalRead,
            buffer.data.size() - totalRead);
        totalRead += socket.read_some(asioBuffer, ec);
        LogError(ec);
      }

      // Call type's `Deserialize`
      size_t bytes = message->Deserialize(buffer);
      if (bytes != header.messageNumBytes - sizeof(header))
      {
        Log::Error("Error while deserializing message");
        return;
      }
      // Write the header that was decoded here into message
      message->BaseData = header;

      // Call `PropagateMessage`
      #define REGISTER(Val) case Message::MessageTypes::Val: \
          PropagateMessage(*(Message::Val*)message.get()); break;
      switch (header.messageType)
      {
        MONS_REGISTER_MESSAGE_TYPES
      }
      #undef REGISTER
    }
  });
}

void Network::LogError(const asio::error_code& error)
{
  if (error)
  {
    Log::Error("Network error: " + error.message());
  }
}

void Network::SendDataRaw(const MessageBuffer& data,
                          id_t machine)
{ 
  // Connect to machine
  asio::io_context context;
  asio::ip::tcp::socket socket(context);
  asio::error_code ec;
  socket.connect(endpoints[machine], ec);
  LogError(ec);
  
  // Send data
  socket.send(asio::buffer(data.data), 0, ec);
  LogError(ec);
  socket.close();
}

// Parse network config
void Network::ParseNetworkConfig()
{
  std::ifstream config;
  config.open("network_config.txt");
  if (config.is_open())
  {
    // Read file line by line
    std::string line;
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
        Log::FatalError("Failed to parse network config line: \"" +
            line + "\".");
      
      // Parse
      uint64_t endpointId;
      uint16_t port;
      asio::ip::address ip;
      try
      {
        endpointId = std::stoul(parts[0]);
      }
      catch (...)
      {
        Log::Error("Failed to parse network ID: \"" + parts[0] + "\".");
      }
      try
      {
        port = std::stoi(parts[2]);
      }
      catch (...)
      {
        Log::Error("Failed to parse port: \"" + parts[2] + "\".");
      }
      asio::error_code ec;
      ip = asio::ip::make_address(parts[1], ec);
      LogError(ec);

      // Add endpoint
      asio::ip::tcp::endpoint endpoint(ip, port);
      AddMachine(endpoint, endpointId);
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

void Network::AddMachine(const asio::ip::tcp::endpoint& endpoint, id_t machine)
{
  if (machine >= endpoints.size())
  {
    endpoints.resize(machine + 1);
  }
  endpoints[machine] = asio::ip::tcp::endpoint(endpoint);
}

#define REGISTER(Val) void Network::PropagateMessage(const Message::Val& message) \
{ \
  auto& callbackList = allCallbacks##Val[message.BaseData.sender]; \
  for (size_t i = 0; i < callbackList.size(); i++) \
    callbackList[i](message); \
}
MONS_REGISTER_MESSAGE_TYPES
#undef REGISTER

} // namespace mons

#endif
