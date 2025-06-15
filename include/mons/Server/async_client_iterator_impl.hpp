#ifndef MONS_SERVER_ASYNC_CLIENT_ITERATOR_IMPL_HPP
#define MONS_SERVER_ASYNC_CLIENT_ITERATOR_IMPL_HPP

#include "async_client_iterator.hpp"

namespace mons {
namespace Server {

void AsyncClientIterator::AddClient(RemoteClient& client)
{
  allClients.push_back(client);
}

void AsyncClientIterator::TryConnect()
{
  connectedClients.clear();
  for (RemoteClient& client : allClients)
  {
    if (!client.IsConnected())
      client.Connect();
    if (client.IsConnected())
      connectedClients.push_back(client);
  }
}

size_t AsyncClientIterator::NumConnected()
{
  return connectedClients.size();
}

void AsyncClientIterator::Iterate(std::function<void(RemoteClient&, size_t i)> function)
{
  // Remove clients that got disconnected
  connectedClients.erase(std::remove_if(connectedClients.begin(),
      connectedClients.end(),
      [&](RemoteClient& client) { return !client.IsConnected(); }),
      connectedClients.end());

  if (connectedClients.size() == 0)
    Log::Error("No connected clients");
  
  // Start running the funcion with each client
  std::vector<std::future<void>> futures;
  for (size_t i = 0; i < connectedClients.size(); i++)
  {
    std::future<void> future = std::async(std::launch::async,
    [this, &function, i]
    {
      function(connectedClients[i], i);
    });
    futures.push_back(std::move(future));
  }

  // Wait for everything to complete
  while (true)
  {
    bool done = true;
    for (std::future<void>& future : futures)
    {
      if (future.valid())
      {
        if (future.wait_for(std::chrono::seconds(1)) ==
            std::future_status::ready)
        {
          future.get();
        }
        else
        {
          done = false;
        }
      }
    }
    if (done)
      break;
  }
}

} // namespace Server
} // namespace mons

#endif
