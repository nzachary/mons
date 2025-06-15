/*
 * Helper to iterate over all available clients
 */
#ifndef MONS_SERVER_ASYNC_CLIENT_ITERATOR_HPP
#define MONS_SERVER_ASYNC_CLIENT_ITERATOR_HPP

#include "../Common/remote_client.hpp"

namespace mons {
namespace Server {

class AsyncClientIterator {
public:
  // Add a client.
  void AddClient(RemoteClient& client);

  // Try to connect to every client that isn't currently connected.
  void TryConnect();

  // Gets the number of connected clients.
  size_t NumConnected();

  // Iterate over connected clients. Blocks until the function has completed.
  // Several instances of function will be run concurrently, be sure it is
  // thread safe.
  void Iterate(std::function<void(RemoteClient&, size_t)> function);
private:
  // All available clients
  std::vector<std::reference_wrapper<RemoteClient>> allClients;

  // Connected clients
  std::vector<std::reference_wrapper<RemoteClient>> connectedClients;
};

} // namespace Server
} // namespace mons

#include "async_client_iterator_impl.hpp"

#endif
