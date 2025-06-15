#ifndef MONS_SERVER_DIST_FUNCTION_SERVER_IMPL_HPP
#define MONS_SERVER_DIST_FUNCTION_SERVER_IMPL_HPP

#include "dist_function_server.hpp"
#include "update_remote_params_callback.hpp"

namespace mons {
namespace Server {

DistFunctionServer::DistFunctionServer(mons::Network& network)
{
  auto& endpoints = network.GetEndpoints();
  for (size_t i = 1; i < endpoints.size(); i++)
  {
    clientIterator.AddClient(RemoteClient::Get(network, i));
  }
}

// Secondary overload, look further down for main overload
template<typename OptimizerType, typename... CallbackTypes>
MONS_ELEM_TYPE
DistFunctionServer::Train(MONS_PREDICTOR_TYPE predictors,
                          MONS_RESPONSE_TYPE responses,
                          CallbackTypes&&... callbacks)
{
  OptimizerType optimizer;
  return Train(std::move(predictors), std::move(responses),
      MONS_WEIGHT_TYPE(), optimizer, callbacks...);
}

// Secondary overload, look further down for main overload
template<typename OptimizerType, typename... CallbackTypes>
MONS_ELEM_TYPE
DistFunctionServer::Train(MONS_PREDICTOR_TYPE predictors,
                          MONS_RESPONSE_TYPE responses,
                          OptimizerType& optimizer,
                          CallbackTypes&&... callbacks)
{
  return Train(std::move(predictors), std::move(responses),
      MONS_WEIGHT_TYPE(), optimizer, callbacks...);
}

// Secondary overload, look further down for main overload
template<typename OptimizerType, typename... CallbackTypes>
MONS_ELEM_TYPE
DistFunctionServer::Train(MONS_PREDICTOR_TYPE predictors,
                          MONS_RESPONSE_TYPE responses,
                          MONS_WEIGHT_TYPE weights,
                          CallbackTypes&&... callbacks)
{
  OptimizerType optimizer;
  return Train(std::move(predictors), std::move(responses),
      MONS_WEIGHT_TYPE(), optimizer, callbacks...);
}

// This is the main implementation
template<typename OptimizerType, typename... CallbackTypes>
MONS_ELEM_TYPE
DistFunctionServer::Train(MONS_PREDICTOR_TYPE predictors,
                          MONS_RESPONSE_TYPE responses,
                          MONS_WEIGHT_TYPE weights,
                          OptimizerType& optimizer,
                          CallbackTypes&&... callbacks)
{
  clientIterator.TryConnect();
  // Prepare function
  function.SetPredictors(predictors);
  function.SetResponses(responses);
  function.SetWeights(weights);
  function._SetInputDims(function.Get().InputDimensions());
  function.Initalize();
  // Initalize clients with the function
  Message::UpdateFunction message;
  Message::Cereal::Cerealize(function.Get(), message.CerealData);
  message.SetInputDimension(function.Get().InputDimensions());
  SafeRef<Message::UpdateFunction> msgRef(message);
  clientIterator.Iterate([&](RemoteClient& client, size_t /* i */)
  {
    int result = client.SendOpWait(msgRef, -1);
    if (result != 0)
      Log::Error("Connection to worker timed out");
  });
  // Send data
  SendData(predictors, responses, weights);
  // Optimize
  const MONS_ELEM_TYPE out =
      optimizer.Optimize(*this, this->Parameters(),
      UpdateRemoteParamsCallback(clientIterator), callbacks...);
  return out;
}

void DistFunctionServer
::SendData(MONS_PREDICTOR_TYPE& predictors,
           MONS_RESPONSE_TYPE& responses,
           MONS_WEIGHT_TYPE& weights)
{
  Message::UpdatePredictors predictorsMessage;
  Message::UpdateResponses responsesMessage;
  Message::UpdateWeights weightsMessage;

  // Write data
  Message::Tensor::SetTensor(predictors, predictorsMessage.TensorData);
  Message::Tensor::SetTensor(responses, responsesMessage.TensorData);
  Message::Tensor::SetTensor(weights, weightsMessage.TensorData);

  // Thread safe references for Iterate
  SafeRef<Message::UpdatePredictors> predictorsRef(predictorsMessage);
  SafeRef<Message::UpdateResponses> responsesRef(responsesMessage);
  SafeRef<Message::UpdateWeights> weightsRef(weightsMessage);

  // Send to clients
  clientIterator.TryConnect();
  clientIterator.Iterate([&](RemoteClient& client, size_t /* i */)
  {
    if (predictors.n_elem > 0)
      client.SendOpWait(predictorsRef);
    if (responses.n_elem > 0)
      client.SendOpWait(responsesRef);
    if (weights.n_elem > 0)
      client.SendOpWait(weightsRef);
  });
}

MONS_ELEM_TYPE DistFunctionServer
::EvaluateWithGradient(const MONS_MAT_TYPE& parameters,
                       const size_t begin,
                       MONS_MAT_TYPE& gradient,
                       const size_t batchSize)
{
  MONS_ELEM_TYPE obj = 0;
  size_t beginTemp = begin;

  // Keep doing until the batch is done
  while (beginTemp < begin + batchSize)
  {
    // Select only the clients that are good
    clientIterator.TryConnect();
    size_t numConnected = clientIterator.NumConnected();
    if (numConnected == 0)
    {
      sleep(10);
      continue;
    }

    // Get batch sizes
    std::vector<size_t> begins(numConnected);
    std::vector<size_t> nCols(numConnected);
    for (size_t i = 0; i < numConnected; i++)
    {
      size_t cols = (batchSize - (beginTemp - begin)) / (numConnected - i);
      begins[i] = beginTemp;
      nCols[i] = cols;
      beginTemp += cols;
    }

    std::mutex mutex;
    // Send to each client
    clientIterator.Iterate([&](RemoteClient& client, size_t i)
    {
      Message::EvaluateWithGradient message;
      Message::Tensor::SetTensor(parameters, message.TensorData);
      message.EvaluateWithGradientData.begin = begins[i];
      message.EvaluateWithGradientData.batchSize = nCols[i];
  
      // Create future
      auto future = client.SendAwaitable
          <Message::EvaluateWithGradient, Message::Gradient>(message);
      if (future.has_value())
      {
        MONS_MAT_TYPE responseGradient;
        Message::Gradient gradientResp = future.value().get();
        Message::Tensor::GetTensor(responseGradient, gradientResp.TensorData);

        std::unique_lock lock(mutex);
        obj += gradientResp.GradientData.objective;
        gradient += responseGradient;
      }
    });
  }

  return obj;
}

void DistFunctionServer::Shuffle()
{
  Message::Shuffle message;
  clientIterator.TryConnect();
  clientIterator.Iterate([&](RemoteClient& client, size_t /* i */)
  {
    client.Send(message);
  });
}

} // namespace Server
} // namespace mons

#endif
