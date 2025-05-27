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
    clients.push_back(RemoteClient::Get(network, i));
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
  initalizedClients.clear();
  // Prepare function
  function.SetPredictors(predictors);
  function.SetResponses(responses);
  function.SetWeights(weights);
  function._SetInputDims(function.Get().InputDimensions());
  function.Initalize();
  // Initalize clients
  Message::UpdateFunction message;
  Message::Cereal::Cerealize(function.Get(), message.CerealData);
  message.SetInputDimension(function.Get().InputDimensions());
  for (RemoteClient& client : clients)
  {
    int result = client.SendOpWait(message, 5);
    if (result == 0)
      initalizedClients.push_back(client);
    else
      Log::Error("Connection to worker timed out");
  }
  // Send data
  SendData(predictors, responses, weights);
  // Optimize
  const MONS_ELEM_TYPE out =
      optimizer.Optimize(*this, this->Parameters(),
      UpdateRemoteParamsCallback(initalizedClients), callbacks...);
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

  // Send to clients
  for (RemoteClient& client : initalizedClients)
  {
    if (predictors.n_elem > 0)
      client.SendOpWait(predictorsMessage);
    if (responses.n_elem > 0)
      client.SendOpWait(responsesMessage);
    if (weights.n_elem > 0)
      client.SendOpWait(weightsMessage);
  }
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
    // Futures for responses from workers
    std::vector<std::future<Message::Gradient>> responses;

    // Select only the clients that are good
    std::vector<std::reference_wrapper<RemoteClient>> goodClients;
    for (size_t i = 0; i < initalizedClients.size(); i++)
    {
      RemoteClient& client = initalizedClients[i];
      // Try connect
      if (!client.IsConnected())
        client.Connect();
      // Only add if connected
      if (client.IsConnected())
        goodClients.push_back(client);
    }
    if (goodClients.size() == 0)
    {
      Log::Error("No connected clients");
      sleep(10);
      continue;
    }

    // Send batches to clients
    for (size_t i = 0; i < goodClients.size(); i++)
    {
      // Split batch size into parts
      size_t nCols = (batchSize - (beginTemp - begin)) / (goodClients.size() - i);
      Message::EvaluateWithGradient message;
      Message::Tensor::SetTensor(parameters, message.TensorData);
      message.EvaluateWithGradientData.begin = beginTemp;
      message.EvaluateWithGradientData.batchSize = nCols;
  
      // Create future
      auto future = goodClients[i].get().SendAwaitable
          <Message::EvaluateWithGradient, Message::Gradient>(message);
      if (future.has_value())
      {
        responses.push_back(std::move(future.value()));
        beginTemp += nCols;
      }
    }

    // Wait on all futures
    arma::wall_clock clock;
    clock.tic();
    while (true)
    {
      // TODO: timeout the infinite loop
      bool done = true;
      for (size_t i = 0; i < responses.size(); i++)
      {
        if (!responses[i].valid())
          continue;
        if (responses[i].wait_for(std::chrono::seconds(1)) ==
            std::future_status::ready)
        {
          MONS_MAT_TYPE responseGradient;
          Message::Gradient gradientResp = responses[i].get();
          Message::Tensor::GetTensor(responseGradient, gradientResp.TensorData);
          obj += gradientResp.GradientData.objective / responses.size();
          gradient += responseGradient;
        }
        else
        {
          done = false;
        }
      }
      if (done)
        break;
    }
  }
  return obj;
}

void DistFunctionServer::Shuffle()
{
  Message::Shuffle message;
  for (RemoteClient& client : initalizedClients)
    client.Send(message);
}

} // namespace Server
} // namespace mons

#endif
