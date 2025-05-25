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
  function._SetInputDims(function.Get().InputDimensions());
  function.Initalize(function.Get());
  // Send function
  Message::UpdateFunction message;
  Message::Cereal::Cerealize(function.Get(), message.CerealData);
  message.SetInputDimension(function.Get().InputDimensions());
  for (RemoteClient& client : clients)
  {
    if (client.SendOpWait(message) == 0)
      initalizedClients.push_back(client);
  }
  // Send data to clients
  ResetData(predictors, responses, weights);
  // Optimize
  const typename MONS_ELEM_TYPE out =
      optimizer.Optimize(*this, this->Parameters(),
      UpdateRemoteParamsCallback(initalizedClients), callbacks...);
  return out;
}

void DistFunctionServer
::ResetData(MONS_PREDICTOR_TYPE predictors,
            MONS_RESPONSE_TYPE responses,
            MONS_WEIGHT_TYPE weights)
{
  numFunctions = responses.n_cols;

  // Send each client their parts
  for (size_t i = 0; i < initalizedClients.size(); i++)
  {
    Message::UpdatePredictors predictorsMessage;
    Message::UpdateResponses responsesMessage;
    Message::UpdateWeights weightsMessage;

    // Write data
    RemoteClient& client = clients[i];
    Message::Tensor::SetTensor(predictors, predictorsMessage.TensorData);
    Message::Tensor::SetTensor(responses, responsesMessage.TensorData);
    Message::Tensor::SetTensor(weights, weightsMessage.TensorData);

    // Send to client
    #ifdef MONS_PREDICTOR_NAME
    client.SendOpWait(predictorsMessage);
    #endif
    #ifdef MONS_RESPONSE_NAME
    client.SendOpWait(responsesMessage);
    #endif
    #ifdef MONS_WEIGHT_NAME
    client.SendOpWait(weightsMessage);
    #endif
  }
}

template <typename T>
void DistFunctionServer
::MakeAliasCols(T& out, T& in, size_t startCol, size_t endCol,
                const typename std::enable_if_t<IsVector<T>::value>*)
{
  mlpack::MakeAlias(out, in, endCol - startCol, startCol);
}

template <typename T>
void DistFunctionServer
::MakeAliasCols(T& out, T& in, size_t startCol, size_t endCol,
                const typename std::enable_if_t<IsMatrix<T>::value>*)
{
  mlpack::MakeAlias(out, in, in.n_rows, endCol - startCol,
      startCol * in.n_rows);
}

template <typename T>
void DistFunctionServer
::MakeAliasCols(T& out, T& in, size_t startCol, size_t endCol,
                const typename std::enable_if_t<IsCube<T>::value>*)
{
  arma::cube a;
  out.resize(in.n_rows, endCol - startCol, in.n_slices);
  for (size_t i = 0; i < in.n_slices; i++)
  {
    MakeAliasCols(out.slice(i), in.slice(i), startCol, endCol);
  }
}


template <typename DataType>
std::vector<DataType> DistFunctionServer::Split(DataType& data, size_t n)
{
  std::vector<DataType> aliases(n);

  size_t begin = 0;
  for (size_t i = 0; i < n; i++)
  {
    size_t nCols = (data.n_cols - begin) / (n - i);
    MakeAliasCols(aliases[i], data, begin, begin + nCols);
    begin += (data.n_cols > 0) ? nCols : 0;
  }

  return std::move(aliases);
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
      bool done = true;
      for (size_t i = 0; i < responses.size(); i++)
      {
        if (!responses[i].valid())
          continue;
        // TODO: timeout length in case it takes longer than 10s to do work
        if (responses[i].wait_for(std::chrono::seconds(10)) ==
            std::future_status::ready)
        {
          MONS_MAT_TYPE responseGradient;
          Message::Gradient gradientResp = responses[i].get();
          Message::Tensor::GetTensor(responseGradient, gradientResp.TensorData);
          obj += gradientResp.GradientData.objective;
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

size_t DistFunctionServer::NumFunctions() const
{
  return numFunctions;
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
