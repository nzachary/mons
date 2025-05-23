#include <iostream>
#include <fstream>
#include <filesystem>

#define MLPACK_ENABLE_ANN_SERIALIZATION
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

  // Set up server
  mons::Server::DistFunctionServer workServer;

  // Set up network
  auto& ffn = workServer.GetFunction();
  ffn.Add<mlpack::Linear>(10);
  ffn.Add<mlpack::Linear>(1);
  ffn.InputDimensions() = {2};

  // Add client
  workServer.AddClient(workerClient);

  // Start training
  MONS_PREDICTOR_TYPE pred(2, 500);
  MONS_RESPONSE_TYPE resp(1, 500);
  for (size_t i = 0; i < 500; i++)
  {
    double theta = 0.01 * i;
    pred(0, i) = std::sin(theta);
    pred(1, i) = std::cos(theta);
    resp(0, i) = theta;
  }
  ens::Adam adam;
  adam.MaxIterations() = 500 * 5;
  workServer.Train(pred, resp, adam, ens::ProgressBar());

  mons::Log::Status("Done!");
  
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
  // Create networks so they can start listening
  mons::Network::Get(0);
  mons::Network::Get(1);

  // Start server and client
  sleep(1);
  std::thread client = std::thread(StartClient);
  std::thread server = std::thread(StartServer);
  
  InfiniteWait();
}
