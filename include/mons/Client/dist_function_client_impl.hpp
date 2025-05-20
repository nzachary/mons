#ifndef MONS_CLIENT_DIST_FUNCTION_CLIENT_IMPL_HPP
#define MONS_CLIENT_DIST_FUNCTION_CLIENT_IMPL_HPP

#include "dist_function_client.hpp"
#include "../Common/Networking/network.hpp"

namespace mons {
namespace Client {

DistFunctionClient
::DistFunctionClient(mons::Common::Networking::Network& network)
{
  using namespace mons::Common::Networking;
  // Register events to update predictors, responses, and weights
  network.RegisterEvent([&](const Message::UpdatePredictors& message)
  {
    message.GetTensor(predictors);
  });
  network.RegisterEvent([&](const Message::UpdateResponses& message)
  {
    message.GetTensor(responses);
  });
  network.RegisterEvent([&](const Message::UpdateWeights& message)
  {
    message.GetTensor(weights);
  });

  // Register functions that can be remotely called
  network.RegisterEvent([&](const Message::Shuffle& message)
  {
    Shuffle();
  });
}

MONS_ELEM_TYPE DistFunctionClient
::EvaluateWithGradient(const MONS_MAT_TYPE& parameters,
                     const size_t begin,
                     MONS_MAT_TYPE& gradient,
                     const size_t batchSize)
{
  return function.EvaluateWithGradient(parameters, begin, gradient, batchSize);
}

void DistFunctionClient::Shuffle()
{
  if (weights.n_elem == 0)
  {
    mlpack::ShuffleData(predictors, responses, predictors, responses);
  }
  else
  {
    mlpack::ShuffleData(predictors, responses, weights,
        predictors, responses, weights);
  }
}

} // namespace Client
} // namespace mons

#endif
