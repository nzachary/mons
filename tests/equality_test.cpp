/*
 * Tests to make sure that mons is equivalent to vanilla mlpack
 */

#define MONS_CUSTOM_CONFIG
#define MONS_MAT_TYPE arma::mat
#define MONS_FUNCTION_TYPE mlpack::FFN<mlpack::MeanSquaredError, mlpack::ConstInitialization>
#define MONS_PREDICTOR_TYPE MONS_MAT_TYPE
#define MONS_RESPONSE_TYPE MONS_MAT_TYPE
#define MONS_WEIGHT_TYPE MONS_SEQUENCE_LENGTH_TYPE

#include "../include/mons.hpp"

#include <fstream>

bool work = true;

void CheckMatrices(MONS_MAT_TYPE& m1, MONS_MAT_TYPE& m2)
{
  assert(m1.n_cols == m2.n_cols);
  assert(m1.n_rows == m2.n_rows);
  assert(m1.n_elem == m2.n_elem);

  for (size_t i = 0; i < m1.n_elem; i++)
  {
    MONS_ELEM_TYPE diff = m1[i] - m2[i];
    assert(std::abs(diff) < 1e-4);
  }
}

void StartClient(int id)
{
  // Create a client on client network and connect to machine 0 (server)
  mons::RemoteClient& serverClient = mons::RemoteClient::Get(mons::Network::Get(id), 0);

  mons::Client::DistFunctionClient worker(serverClient);
  
  // Wait infinitely
  // Everything else is handled by the worker
  // We just need to keep it in scope
  while (work)
  {
    sleep(1);
  }
}

void TestEquivalence()
{
  // Initalize mlpack network
  MONS_FUNCTION_TYPE mlpackNetwork;
  mlpackNetwork.Add<mlpack::Linear>(50);
  mlpackNetwork.Add<mlpack::Sigmoid>();
  mlpackNetwork.Add<mlpack::Linear>(1);
  mlpackNetwork.InputDimensions() = {2};

  // Initalize mons network
  mons::Server::DistFunctionServer monsServer(mons::Network::Get(0));
  // Copy mlpack network
  MONS_FUNCTION_TYPE& monsNetwork = monsServer.GetFunction();
  monsNetwork.Add<mlpack::Linear>(50);
  monsNetwork.Add<mlpack::Sigmoid>();
  monsNetwork.Add<mlpack::Linear>(1);
  monsNetwork.InputDimensions() = {2};

  // Generate data
  MONS_PREDICTOR_TYPE pred(2, 500, arma::fill::randu);
  MONS_RESPONSE_TYPE resp(1, 500, arma::fill::randu);

  // Start training
  ens::Adam adam;
  adam.MaxIterations() = 500;
  adam.BatchSize() = 250;

  mons::Log::Debug("Training mlpack network");
  mlpackNetwork.Train(pred, resp, adam, ens::ProgressBar());
  mons::Log::Debug("Training mons network");
  monsServer.Train(pred, resp, adam, ens::ProgressBar());

  // Check equivalence
  CheckMatrices(mlpackNetwork.Parameters(),
      monsNetwork.Parameters());
}

int main()
{
  // Create network config
  {
    std::ofstream newConfig("network_config.txt");
    newConfig << "0;127.0.0.1;1337\n1;127.0.0.1;1338\n2;127.0.0.1;1339\n3;127.0.0.1;1340\n4;127.0.0.1;1341";
  }

  std::vector<std::thread> clientThreads(4);
  // Initalize clients
  for (size_t i = 0; i < clientThreads.size(); i++)
  {
    clientThreads[i] = std::thread(StartClient, i + 1);
  }

  // Run tests
  TestEquivalence();
  mons::Log::Status("Equivalence test passed");

  // Terminate clients
  work = false;
  for (size_t i = 0; i < clientThreads.size(); i++)
    clientThreads[i].join();
}
