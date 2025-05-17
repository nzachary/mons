#ifndef MONS_COMMON_NETWORK_IMPL_HPP
#define MONS_COMMON_NETWORK_IMPL_HPP

#include "network.hpp"

#include <fstream>

#include "../log.hpp"
#include "Message/message_types.hpp"

namespace mons {
namespace Common {
namespace Networking {

Network::Network(id_t id) : id(id)
{
  ParseNetworkConfig();

  // Start listener thread
  listenerThread = std::thread([&]()
  {
    while (true) {
      const std::shared_ptr<Message::NetworkMessage> msg = Recieve();

      // Call `PropagateMessage` for the first type it successfully casts to
      #define REGISTER(Val) \
      if (Message::Val* msgPtr = dynamic_cast<Message::Val*>(msg.get())) \
      { \
        PropagateMessage(*msgPtr); \
        continue; \
      }
      MONS_REGISTER_MESSAGE_TYPES
      #undef REGISTER
    }
  });

  // Add heartbeat listener to bump `lastHeartbeat`
  RegisterEvent([&](const Message::HeartbeatMessage& message)
  {
    id_t sender = message.NetworkMessageData.sender;
    if (sender > lastHeartbeat.size())
    {
      Log::Error("Recieved message from invalid machine " +
          std::to_string(sender) + ".");
    }
    else
    {
      lastHeartbeat[sender] = std::chrono::system_clock::now();
    }
  });

  // Start thread to send out heartbeats
  heartThread = std::thread([&]()
  {
    std::shared_ptr<Message::HeartbeatMessage> beat =
        std::make_shared<Message::HeartbeatMessage>();
    while (true)
    {
      auto now = std::chrono::system_clock::now();
      for (const id_t& id : connected)
      {
        Send(beat, id);
      }

      beat->HeartbeatMessageData.beatCount++;

      sleep(5);
    }
  });

  // If this is a client, send heartbeats to server
  if (id != 0)
    connected.push_back(0);
}

void Network::Send(std::shared_ptr<Message::NetworkMessage> message,
                   id_t machine)
{
  // Set message sender/reciever
  message->NetworkMessageData.sender = id;
  message->NetworkMessageData.reciever = machine;
  // Send message
  std::vector<char> buffer = message->Serialize();
  SendDataRaw(buffer, machine);
}

#define REGISTER(Val) \
void Network::RegisterEvent(std::function<void(const Message::Val&)> callback) \
{ \
  allCallbacks##Val.push_back(std::move(callback)); \
}
MONS_REGISTER_MESSAGE_TYPES
#undef REGISTER

std::shared_ptr<Message::NetworkMessage> Network::Recieve()
{
  // Accept incoming connection
  asio::io_context context;
  asio::ip::tcp::acceptor acceptor(context, endpoints[id]);
  asio::ip::tcp::socket socket(context);
  asio::error_code ec;
  acceptor.accept(socket, ec);
  LogError(ec);

  // Recieve header
  Message::NetworkMessage::NetworkMessageDataStruct header;
  std::vector<char> buffer(sizeof(header));
  socket.read_some(asio::buffer(buffer), ec);
  LogError(ec);

  // Try to decode the header
  header = Message::NetworkMessage::DecodeHeader(buffer);

  // Check message API version
  if (header.apiVersion != MONS_VERSION_NUMBER) {
    Log::Error("Recieved message with version " +
        std::to_string(header.apiVersion) +
        " but expected " + std::to_string(MONS_VERSION_NUMBER));
    return nullptr;
  }

  // Create a shared_ptr with the correct underlying class
  std::shared_ptr<Message::NetworkMessage> message(nullptr);
  #define REGISTER(Val) case Message::MessageTypes::Val: \
      message = std::make_shared<Message::Val>(); break;
  switch (header.messageType)
  {
    MONS_REGISTER_MESSAGE_TYPES
  }
  #undef REGISTER

  // Read the rest of the message
  buffer.resize(header.messageNumBytes - sizeof(header));
  size_t totalRead = 0;
  while (totalRead < buffer.size())
  {
    auto asioBuffer = asio::buffer(buffer.data() + totalRead,
        buffer.size() - totalRead);
    totalRead += socket.read_some(asioBuffer, ec);
    LogError(ec);
  }

  // Call type's `Deserialize`
  message->Deserialize(buffer);
  // Write the header that was decoded here into message
  message->NetworkMessageData = header;

  // Return the parsed message
  return message;
}

void Network::LogError(const asio::error_code& error)
{
  if (error)
  {
    Log::Error("Network error: " + error.message());
  }
}

void Network::SendDataRaw(const std::vector<char>& data, id_t machine)
{
  // Connect to machine
  asio::io_context context;
  asio::ip::tcp::socket socket(context);
  asio::error_code ec;
  socket.connect(endpoints[machine], ec);
  LogError(ec);
  
  // Send data
  socket.send(asio::buffer(data), 0, ec);
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

void Network::AddMachine(const asio::ip::tcp::endpoint& endpoint, id_t machine) {
  if (machine >= endpoints.size())
  {
    endpoints.resize(machine + 1);
    lastHeartbeat.resize(machine + 1);
  }
  endpoints[machine] = asio::ip::tcp::endpoint(endpoint);
}

void Network::AddUniqueConnected(id_t machine)
{
  // Check if it exists
  bool exists = false;
  for (id_t i : connected)
  {
    if (machine == i)
    {
      exists = true;
      break;
    }
  }
  // Add to tracking list
  if (!exists)
  {
    Log::Status("New connection to machine " + std::to_string(machine));
    connected.push_back(machine);
  }
}

#define REGISTER(Val) void Network::PropagateMessage(const Message::Val& message) \
{ \
  for (size_t i = 0; i < allCallbacks##Val.size(); i++) \
    allCallbacks##Val[i](message); \
}
MONS_REGISTER_MESSAGE_TYPES
#undef REGISTER

} // namespace Networking
} // namespace Common
} // namespace mons

#endif
