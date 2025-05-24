#ifndef MONS_COMMON_SOCKET_IMPL_HPP
#define MONS_COMMON_SOCKET_IMPL_HPP

#include "socket.hpp"

namespace mons {

Socket::Socket(asio::ip::tcp::endpoint local,
               asio::ip::tcp::endpoint remote)
: initLocal(local), initRemote(remote)
{
  _context = std::make_unique<asio::io_context>();
  auto work = asio::require(_context->get_executor(),
      asio::execution::outstanding_work.tracked);
  std::thread contextRuner([this]() { _context->run(); });
  contextRuner.detach();
}

Socket::~Socket()
{
  _context->stop();
}


bool Socket::IsConnected()
{
  return _context && _socket;
}

bool Socket::Listen()
{
  std::unique_lock sl(sm);
  std::unique_lock rl(rm);
  Log::Debug("Starting listener on port " + std::to_string(initLocal.port()));
  asio::error_code ec;
  // Create and accept sockets until we get a connection from `from`
  std::unique_ptr<asio::ip::tcp::socket> connectionSocket =
      AcceptFrom(initLocal, initRemote, *_context);

  Log::Debug("Recieved connection request");

  // Create a new socket to get a new random port
  asio::ip::tcp::endpoint newEndpoint(initLocal.address(), 0);
  {
    asio::ip::tcp::socket newSocket(*_context, newEndpoint);
    newEndpoint = newSocket.local_endpoint();
  }

  // Accept connection on the new port
  std::thread acceptThread([&]()
  {
    Log::Debug("Opened new socket on port " + std::to_string(newEndpoint.port()));
    _socket = AcceptFrom(newEndpoint, initRemote, *_context);
    Log::Debug("Connected remote on port " + std::to_string(newEndpoint.port()));
  });
  
  // TODO: better timing
  // If the listener isn't ready when connecting client recieves the new port,
  // 1) The incoming connection is refused and 2) Listener is stuck waiting
  sleep(1);

  // Send port of socket to peer so it can connect to the new socket
  MessageBuffer buf(0);
  Serialize(buf, newEndpoint.port(), true);
  SocketSend(buf, *connectionSocket, ec);
  if (ec)
    return false;

  // Wait for connection connection
  acceptThread.join();
  return true;
}

bool Socket::Connect(size_t retry)
{
  std::unique_lock sl(sm);
  std::unique_lock rl(rm);
  // Connect to remote host's listening port
  // TODO: timeout
  asio::error_code ec;
  Log::Debug("Attempting connection to " + initRemote.address().to_string() +
      ":" + std::to_string(initRemote.port()));
  for (size_t i = 0; i < retry; i++)
  {
    ec.clear();
    std::unique_ptr<asio::ip::tcp::socket> tempSocket =
        std::make_unique<asio::ip::tcp::socket>(*_context);
    tempSocket->connect(initRemote, ec);
    if (ec)
      continue;
    
    // Recieve new port to establish a more permanent connection on
    MessageBuffer buf(0);
    SocketRecieve(buf, *tempSocket, ec);
    if (ec)
      continue;
    uint16_t newPort;
    Serialize(buf, newPort, false);

    // Connect to new port
    tempSocket->close();
    asio::ip::tcp::endpoint newEndpoint(initRemote.address(), newPort);
    Log::Debug("Attempting connection to " + initRemote.address().to_string() +
        ":" + std::to_string(newPort) + " (original " +
        std::to_string(initRemote.port()) + ")");
    tempSocket->connect(newEndpoint, ec);
    if (ec)
      continue;
    
      // Finalize
    _socket = std::move(tempSocket);
    Log::Debug("Connected to " + initRemote.address().to_string() + ":" +
        std::to_string(initRemote.port()) + " via port " +
        std::to_string(newPort));
    return true;
  }
  Log::Error("Connection error: " + ec.message());
  return false;
}

void Socket::Disconnect()
{
  std::unique_lock sl(sm);
  std::unique_lock rl(rm);
  _socket.reset();
  _socket = std::make_unique<asio::ip::tcp::socket>(nullptr);
}

bool Socket::Send(const MessageBuffer& buf, size_t retry)
{
  std::unique_lock sl(sm);
  asio::error_code ec;
  for (size_t i = 0; i < retry; i++)
  {
    ec.clear();
    SocketSend(buf, *_socket, ec);
    if (!ec)
      return true;
    sleep(1);
  }
  Log::Error("Error sending: sent " + std::to_string(buf.data.size()));
  Disconnect();
  return false;
}

bool Socket::Recieve(MessageBuffer& buf, size_t retry)
{
  std::unique_lock rl(rm);
  asio::error_code ec;
  for (size_t i = 0; i < retry; i++)
  {
    ec.clear();
    SocketRecieve(buf, *_socket, ec);
    if (!ec)
      return true;
    sleep(1);
  }
  Log::Error("Error recieving: recieved " + std::to_string(buf.data.size()));
  Disconnect();
  return false;
}

std::unique_ptr<asio::ip::tcp::socket> Socket
::AcceptFrom(asio::ip::tcp::endpoint local,
             asio::ip::tcp::endpoint remote,
             asio::io_context& context)
{
  asio::error_code ec;
  // Create and accept sockets until we get a connection from `from`
  while (true)
  {
    ec.clear();
    asio::ip::tcp::acceptor acceptor(context, local);
    asio::ip::tcp::endpoint requestPeer;
    std::unique_ptr<asio::ip::tcp::socket> tempSocket =
        std::make_unique<asio::ip::tcp::socket>(context);
    acceptor.accept(*tempSocket, requestPeer, ec);
    if (ec)
      continue;
    if (requestPeer.address() == remote.address())
    {
      return tempSocket;
    }
  }
}

void Socket::SocketSend(const MessageBuffer& buf,
                        asio::ip::tcp::socket& socket,
                        asio::error_code& ec,
                        double timeout)
{
  // Prefix buffer with length information
  int64_t totalLen = buf.data.size() + sizeof(totalLen);
  MessageBuffer newBuf(0);
  Serialize(newBuf, totalLen, true, 0);
  newBuf.data.insert(newBuf.data.end(), buf.data.begin(), buf.data.end());
  // Send newly prefixed buffer
  size_t totalSent = 0;
  while (totalSent < totalLen)
  {
    auto asioBuffer = asio::buffer(newBuf.data.data() + totalSent,
        newBuf.data.size() - totalSent);
    // Write some amount or timeout
    std::future<size_t> reader = std::async([&]()
    { return socket.write_some(asioBuffer, ec); });
    std::future_status status = reader.wait_for(
        std::chrono::milliseconds((size_t)(1000 * timeout)));
    if (!ec && status == std::future_status::ready)
      totalSent += reader.get();

    if (ec)
      return;
  }
}

bool Socket::SocketRecieve(MessageBuffer& buf,
                           asio::ip::tcp::socket& socket,
                           asio::error_code& ec,
                           double timeout)
{
  int64_t totalLen = -1;
  size_t totalRead = 0;
  // Read length information
  buf.data.resize(0);
  // Read the message
  while (totalRead < totalLen || totalLen < 0)
  {
    // Prevent overshooting
    const size_t readLength = (totalLen > 0) ? totalLen - totalRead :
        sizeof(totalLen);
    buf.data.resize(totalRead + readLength);
    auto asioBuffer = asio::buffer(buf.data.data() + totalRead,
        readLength);
    // Read some amount or timeout
    std::future<size_t> reader = std::async([&]()
    { return socket.read_some(asioBuffer, ec); });
    std::future_status status = reader.wait_for(
        std::chrono::milliseconds((size_t)(1000 * timeout)));
    if (!ec && status == std::future_status::ready)
      totalRead += reader.get();
      
    // Get total length if it isn't known yet
    if (totalLen < 0 && totalRead >= sizeof(totalLen))
    {
      Serialize(buf, totalLen, false);
      buf.deserializePtr = 0;
      buf.processed = 0;
    }
    if (ec)
      break;
  }
  // Remove trailing empty space
  buf.data.resize(totalRead);
  // Remove length information
  buf.data.erase(buf.data.begin(), buf.data.begin() + sizeof(totalLen));
  if (ec == asio::error::eof)
    ec.clear();
  return (bool)ec;
}

}

#endif
