#ifndef MONS_SERVER_DIST_FUNCTION_SERVER_IMPL_HPP
#define MONS_SERVER_DIST_FUNCTION_SERVER_IMPL_HPP

#include "dist_function_server.hpp"

namespace mons {
namespace Server {

// Add a worker client
void DistFunctionServer::AddClient(RemoteClient& client)
{
  clients.push_back(client);
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
  // Prepare function
  function._SetInputDims(function.Get().InputDimensions());
  function.Initalize(function.Get());
  // Send function
  Message::UpdateFunction message;
  Message::Cereal::Cerealize(function.Get(), message.CerealData);
  message.SetInputDimension(function.Get().InputDimensions());
  for (RemoteClient& client : clients)
    client.SendOpWait(message);
  // Send data
  // Sending after function so that they aren't reset
  ResetData(predictors, responses, weights);
  // Optimize
  const typename MONS_ELEM_TYPE out =
      optimizer.Optimize(*this, function.Get().Parameters(), callbacks...);
  return out;
}

void DistFunctionServer
::ResetData(MONS_PREDICTOR_TYPE predictors,
            MONS_RESPONSE_TYPE responses,
            MONS_WEIGHT_TYPE weights)
{
  numFunctions = responses.n_cols;

  // Split parameters into parts
  std::vector<MONS_PREDICTOR_TYPE> predictorAliases =
      Split(predictors, clients.size());
  std::vector<MONS_RESPONSE_TYPE> responseAliases =
      Split(responses, clients.size());
  std::vector<MONS_WEIGHT_TYPE> weightAliases =
      Split(weights, clients.size());
  // Send each client their parts
  for (size_t i = 0; i < clients.size(); i++)
  {
    Message::UpdatePredictors predictorsMessage;
    Message::UpdateResponses responsesMessage;
    Message::UpdateWeights weightsMessage;

    // Write data
    RemoteClient& client = clients[i];
    Message::Tensor::SetTensor(predictorAliases[i],
        predictorsMessage.TensorData);
    Message::Tensor::SetTensor(responseAliases[i],
        responsesMessage.TensorData);
    Message::Tensor::SetTensor(weightAliases[i],
        weightsMessage.TensorData);

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
    begin += nCols + 1;
  }

  return std::move(aliases);
}

MONS_ELEM_TYPE DistFunctionServer
::EvaluateWithGradient(const MONS_MAT_TYPE& parameters,
                       const size_t begin,
                       MONS_MAT_TYPE& gradient,
                       const size_t batchSize)
{
  size_t beginTemp = begin;
  // Futures for responses from workers
  std::vector<std::future<Message::Gradient>> responses(clients.size());

  for (size_t i = 0; i < clients.size(); i++)
  {
    // Split batch size into parts
    size_t nCols = (batchSize - (begin - beginTemp)) / (clients.size() - i);
    Message::EvaluateWithGradient message;
    Message::Tensor::SetTensor(parameters, message.TensorData);
    message.EvaluateWithGradientData.begin = begin;
    message.EvaluateWithGradientData.batchSize = nCols;

    // Create future
    responses[i] = clients[i].get().SendAwaitable
        <Message::EvaluateWithGradient, Message::Gradient>(message);
    beginTemp += nCols + 1;
  }

  // Wait on all futures
  arma::wall_clock clock;
  clock.tic();
  MONS_ELEM_TYPE obj = 0;
  while (true)
  {
    // TODO: timeout
    bool done = true;
    for (size_t i = 0; i < responses.size(); i++)
    {
      if (!responses[i].valid())
        continue;
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
      sleep(1);
    }
    if (done)
      break;
  }
  return obj;
}

size_t DistFunctionServer::NumFunctions() const
{
  return numFunctions;
}

void DistFunctionServer::Shuffle()
{
  
}

} // namespace Server
} // namespace mons

#endif
