#include "mons_config.hpp"
#include "../../include/mons.hpp"

int main()
{
  // Set up server
  mons::Network& network = mons::Network::Get(0);
  mons::Server::DistFunctionServer workServer(network);

  // Set up FFN
  auto& ffn = workServer.GetFunction();
  ffn.Add<mlpack::Linear>(1024);
  ffn.Add<mlpack::ReLU>();
  ffn.Add<mlpack::Linear>(1024);
  ffn.Add<mlpack::ReLU>();
  ffn.Add<mlpack::Linear>(1);
  ffn.InputDimensions() = {2};

  // Create training data
  MONS_PREDICTOR_TYPE pred(2, 500);
  MONS_RESPONSE_TYPE resp(1, 500);
  for (size_t i = 0; i < 500; i++)
  {
    double theta = 0.01 * i;
    pred(0, i) = std::sin(theta);
    pred(1, i) = std::cos(theta);
    resp(0, i) = std::fmod(theta, 2 * M_PI);
  }

  // Start training
  ens::Adam adam;
  adam.MaxIterations() = 500 * 100;
  adam.BatchSize() = 100;
  adam.StepSize() = 1e-6;

  workServer.Train(pred, resp, adam, ens::ProgressBar());

  std::printf("Done!\n");
}
