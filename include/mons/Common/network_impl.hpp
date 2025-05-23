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
  MessageBuffer buffer = message.Message::Base::Serialize();
  // Send message
  SendDataRaw(buffer, machine);
}

template <typename MessageType, typename ResponseType>
std::future<ResponseType> Network
::SendAwaitable(MessageType& message,
                id_t machine)
{
  // Set message ID
  message.BaseData.id = idCounter++;

  // Launch waiter
  uint64_t messageId = message.BaseData.id;
  std::future<ResponseType> waitable = std::async(std::launch::async,
      [this, messageId, machine]()
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
    // Wait for event to trigger
    std::unique_lock lock(m);
    cv.wait(lock, [&]{ return cvPayload; });
    return response;
  });

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
      asio::ip::tcp::socket socket(context);
      asio::ip::tcp::acceptor acceptor(context, endpoints[id]);
      asio::error_code ec;
      acceptor.accept(socket, ec);
      LogError(ec);

      // Recieve message
      size_t totalRead = 0;
      MessageBuffer buffer(0);
      while (true)
      {
        const size_t BUFFER_INCREMENT = 128;
        buffer.data.resize(buffer.data.size() + BUFFER_INCREMENT);
        auto asioBuffer = asio::buffer(buffer.data.data() + totalRead,
            BUFFER_INCREMENT);
        totalRead += socket.read_some(asioBuffer, ec);
        if (ec == asio::error::eof)
          // Done recieving
          break;
        else
          LogError(ec);
      }
      socket.close();

      // Resize buffer down to remove trailing empty data
      buffer.data.resize(totalRead);
      buffer.deserializePtr = 0;

      // Try to decode the header
      Message::Base::BaseDataStruct header;
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

      // Write the header that was decoded earlier into message
      message->BaseData = header;

      // Call type's `Deserialize` and read the rest
      size_t bytes = message->Deserialize(buffer);
      if (bytes != header.messageNumBytes)
      {
        Log::Error("Error while deserializing message");
        return;
      }

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
  auto lock = std::unique_lock(networkMutex);
  // Connect to machine
  asio::io_context context;
  asio::ip::tcp::socket socket(context);
  asio::error_code ec;
  socket.connect(endpoints[machine], ec);
  LogError(ec);
  
  // Send data
  size_t totalSent = 0;
  while (totalSent < data.data.size())
  {
    auto asioBuffer = asio::buffer(data.data.data() + totalSent,
        data.data.size() - totalSent);
    totalSent += socket.write_some(asioBuffer, ec);
    LogError(ec);
  }
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
  auto& cb = allCallbacks##Val[message.BaseData.sender]; \
  cb.erase(std::remove_if(cb.begin(), cb.end(), \
      [&](std::function<bool(const Message::Val&)>& callback) \
      { return callback(message); }), cb.end()); \
}
MONS_REGISTER_MESSAGE_TYPES
#undef REGISTER

} // namespace mons

#endif
