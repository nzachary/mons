#include <iostream>
#include <fstream>
#include <filesystem>

#include "../include/mons.hpp"

void InfiniteWait()
{
  while (true)
  {
    sleep(10);
  }
}

void StartServer()
{
  mons::Network& network = mons::Network::Get(0);
  // Create a client on server network and connect to machine 1 (worker)
  mons::RemoteClient& workerClient = mons::RemoteClient::Get(network, 1);

  workerClient.OnRecieve([](const mons::Message::Heartbeat& message)
  {
    std::cout << "Recieved: " << message.HeartbeatData.beatCount << std::endl;
  });
  
  InfiniteWait();
}

void StartClient()
{
  mons::Network& network = mons::Network::Get(1);
  // Create a client on client network and connect to machine 0 (server)
  mons::RemoteClient& serverClient = mons::RemoteClient::Get(network, 0);

  mons::Client::DistFunctionClient worker(serverClient);
  
  InfiniteWait();
}

int main()
{
  // Create basic config
  if (!std::filesystem::exists("network_config.txt"))
  {
    std::ofstream newConfig("network_config.txt");
    if (!newConfig.is_open())
    {
      mons::Log::FatalError("Failed to create network config");
    }
    newConfig << "0;127.0.0.1;1337\n1;127.0.0.1;1338";
  }
  
  std::thread server = std::thread(StartServer);
  std::thread client = std::thread(StartClient);
  
  InfiniteWait();
}
