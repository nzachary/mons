#include <iostream>
#include <fstream>
#include <filesystem>

#include "../include/mons.hpp"

bool work = true;

void StartServer()
{
  mons::Network& network = mons::Network::Get(0);

  // Set up server
  mons::Server::DistFunctionServer workServer;

  // Set up network
  auto& ffn = workServer.GetFunction();
  ffn.Add<mlpack::Linear>(20);
  ffn.Add<mlpack::Linear>(1);
  ffn.InputDimensions() = {2};

  // Add clients
  workServer.AddClient(mons::RemoteClient::Get(network, 1));
  workServer.AddClient(mons::RemoteClient::Get(network, 2));
  workServer.AddClient(mons::RemoteClient::Get(network, 3));

  // Start training
  MONS_PREDICTOR_TYPE pred(2, 5000);
  MONS_RESPONSE_TYPE resp(1, 5000);
  for (size_t i = 0; i < 5000; i++)
  {
    double theta = 0.01 * i;
    pred(0, i) = std::sin(theta);
    pred(1, i) = std::cos(theta);
    resp(0, i) = std::fmod(theta, 2 * M_PI);
  }
  ens::Adam adam;
  adam.MaxIterations() = 5000 * 5;
  adam.StepSize() = 1e-6;

  workServer.Train(pred, resp, adam, ens::ProgressBar());

  mons::Log::Status("Done!");
}

void StartClient(int id)
{
  mons::Network& network = mons::Network::Get(id);
  // Create a client on client network and connect to machine 0 (server)
  mons::RemoteClient& serverClient = mons::RemoteClient::Get(network, 0);

  mons::Client::DistFunctionClient worker(serverClient);
  
  // Wait infinitely
  // Everything else is handled by the worker
  // We just need to keep it in scope
  while (work)
  {
    sleep(1);
  }
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
    newConfig << "0;127.0.0.1;1337\n1;127.0.0.1;1338\n2;127.0.0.1;1339\n3;127.0.0.1;1340";
  }
  // Start clients
  std::thread client1 = std::thread(StartClient, 1);
  std::thread client2 = std::thread(StartClient, 2);
  std::thread client3 = std::thread(StartClient, 3);

  // Start server
  sleep(1);
  mons::Network::Get(0);
  StartServer();

  // Terminate clients
  work = false;

  client1.join();
  client2.join();
  client3.join();
}
