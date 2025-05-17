#include <iostream>

#include "../include/mons.hpp"

using namespace mons::Common::Networking;
using namespace mons::Client;

void InfiniteWait()
{
  while (true)
  {
    sleep(10);
  }
}

void StartServer()
{
  Network serverNetwork(0);

  serverNetwork.RegisterEvent([](const Message::HeartbeatMessage& message)
  {
    std::cout << "Recieved: " << message.HeartbeatMessageData.beatCount << std::endl;
  });
  
  InfiniteWait();
}

void StartClient()
{
  Network clientNetwork(1);

  DistFunctionClient worker(clientNetwork);
  
  InfiniteWait();
}

int main()
{
  std::thread server = std::thread(StartServer);
  std::thread client = std::thread(StartClient);
  
  InfiniteWait();
}
