#include <iostream>
#include <fstream>
#include <filesystem>

#include "mons_config.hpp"
#include "../../include/mons.hpp"

int main(int argc, char* argv[])
{
  int id;
  if (argc == 1)
    id = std::stoi(argv[0]);
  else
  {
    std::printf("Usage: ./ffn-client {ID}\n");
    return 0;
  }
  // Create a client and connect to machine 0 (server)
  mons::Network& network = mons::Network::Get(id);
  mons::RemoteClient& serverClient = mons::RemoteClient::Get(network, 0);

  // Start worker client
  mons::Client::DistFunctionClient worker(serverClient);
  
  // Wait infinitely
  // Everything else is handled by the worker
  // We just need to keep it in scope
  while (true)
  {
    sleep(1);
  }
}
