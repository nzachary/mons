/*
 * Tests to make sure that all data is trasmitted correctly through a network
 * without loss of data or corruption
 */

#include "../include/mons.hpp"

#include <fstream>

void CreateNetworkConfig()
{
  std::ofstream newConfig("network_config.txt");
  newConfig << "0;127.0.0.1;1337\n1;127.0.0.1;1338";
}

// Test sending a large amount of random data over a network
void TestRandomData(mons::RemoteClient& remote1, mons::RemoteClient& remote0)
{
  arma::cube data(100, 100, 100);
  data.randn();
  mons::Message::UpdateParameters message;
  mons::Message::Tensor::SetTensor(data, message.TensorData);

  bool done = false;

  remote1.OnRecieve([&](const mons::Message::UpdateParameters& rec)
  {
    arma::cube recData;
    mons::Message::Tensor::GetTensor(recData, rec.TensorData);
    assert(recData.n_rows == data.n_rows);
    assert(recData.n_cols == data.n_cols);
    assert(recData.n_slices == data.n_slices);
    for (size_t r = 0; r < data.n_rows; r++)
    {
      for (size_t c = 0; c < data.n_rows; c++)
      {
        for (size_t s = 0; s < data.n_rows; s++)
        {
          double diff = recData(r, c, s) - data(r, c, s);
          assert(std::abs(diff) < 1e-5);
        }
      }
    }

    mons::Log::Status("Network random data test passed");

    done = true;

    return true;
  });
  mons::Log::Debug("Sending data");
  remote0.Send(message);
  mons::Log::Debug("Waiting on recieve");
  while (!done)
    sleep(1);
}

int main()
{
  CreateNetworkConfig();

  // Initialize remote0
  // Do this asynchronously since `RemoteClient::Connect` (which the constructor calls) blocks
  std::future a1 = std::async(std::launch::async,([](){
    mons::RemoteClient::Get(mons::Network::Get(1), 0);
  })); 
  mons::RemoteClient& remote1 = mons::RemoteClient::Get(mons::Network::Get(0), 1);
  a1.wait();
  mons::RemoteClient& remote0 = mons::RemoteClient::Get(mons::Network::Get(1), 0);

  for (size_t i = 0; i < 5; i++)
  {
    TestRandomData(remote1, remote0);
  }
}
