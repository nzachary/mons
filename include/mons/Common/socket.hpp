/*
 * A network I/O socket
 */
#ifndef MONS_COMMON_SOCKET_HPP
#define MONS_COMMON_SOCKET_HPP

#include <asio.hpp>

#include "serialize.hpp"

namespace mons {

class Socket
{
public:
  Socket(asio::ip::tcp::endpoint local,
         asio::ip::tcp::endpoint remote);
  ~Socket();

  // Check if the socket is connected to another socket
  bool IsConnected();

  // Wait for an incoming connection
  // There should only be one listener per port
  bool Listen();

  // Connect to the remote host
  bool Connect(size_t retry = 5);

  // Disconnect from the remote host
  void Disconnect();

  // Send a message to the remote client, can run in parallel with recieve
  // Only one message is sent at a time
  bool Send(const MessageBuffer& buf, size_t retry = 5);

  // Recieve a message from the remote host, can run in parallel with send
  // Only one message is recieved at a time
  bool Recieve(MessageBuffer& buf, size_t retry = 5);
private:
  // Accept a connection coming from `remote` to local endpoint `local`
  static std::unique_ptr<asio::ip::tcp::socket>
  AcceptFrom(asio::ip::tcp::endpoint local,
             asio::ip::tcp::endpoint remote,
             asio::io_context& context);
  // Send data over a socket
  static void SocketSend(const MessageBuffer& buf,
                         asio::ip::tcp::socket& socket,
                         asio::error_code& ec,
                         double timeout = 5);
  // Recieve data over a socket
  static bool SocketRecieve(MessageBuffer& buf,
                            asio::ip::tcp::socket& socket,
                            asio::error_code& ec,
                            double timeout = 5);

  // Mutexes for locking send and recieve
  std::mutex sm, rm;
  // asio context
  std::unique_ptr<asio::io_context> _context;
  // asio socket
  std::unique_ptr<asio::ip::tcp::socket> _socket;
  // Local endpoint for initating a connection
  asio::ip::tcp::endpoint initLocal;
  // Remote endpoint for initating a connection
  asio::ip::tcp::endpoint initRemote;
};

}

#include "socket_impl.hpp"

#endif
