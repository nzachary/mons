#ifndef MONS_SERVER_DIST_FUNCTION_SERVER_IMPL_HPP
#define MONS_SERVER_DIST_FUNCTION_SERVER_IMPL_HPP

#include "dist_function_server.hpp"

namespace mons {
namespace Server {

template<typename OptimizerType, typename... CallbackTypes>
MONS_ELEM_TYPE
DistFunctionServer::Train(MONS_PREDICTOR_TYPE predictors,
                          MONS_RESPONSE_TYPE responses,
                          CallbackTypes&&... callbacks)
{
  OptimizerType optimizer;
  return Train(std::move(predictors), std::move(responses),
      MONS_WEIGHT_TYPE(), optimizer, callbacks);
}

template<typename OptimizerType, typename... CallbackTypes>
MONS_ELEM_TYPE
DistFunctionServer::Train(MONS_PREDICTOR_TYPE predictors,
                          MONS_RESPONSE_TYPE responses,
                          OptimizerType& optimizer,
                          CallbackTypes&&... callbacks)
{
  return Train(std::move(predictors), std::move(responses),
      MONS_WEIGHT_TYPE(), optimizer, callbacks);
}

template<typename OptimizerType, typename... CallbackTypes>
MONS_ELEM_TYPE
DistFunctionServer::Train(MONS_PREDICTOR_TYPE predictors,
                          MONS_RESPONSE_TYPE responses,
                          MONS_WEIGHT_TYPE weights,
                          CallbackTypes&&... callbacks)
{
  OptimizerType optimizer;
  return Train(std::move(predictors), std::move(responses),
      MONS_WEIGHT_TYPE(), optimizer, callbacks);
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
  ResetData(predictors, responses, weights);
  const typename MatType::elem_type out =
      optimizer.Optimize(*this, function.parameters(), callbacks...);
  return out;
}

void DistFunctionServer
::ResetData(MONS_PREDICTOR_TYPE predictors,
             MONS_RESPONSE_TYPE responses,
             MONS_WEIGHT_TYPE weights = MONS_WEIGHT_TYPE())
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
    id_t client = clients[i];
    predictorsMessage.SetTensor(predictorAliases[i]);
    responsesMessage.SetTensor(responseAliases[i]);
    weightsMessage.SetTensor(weightAliases[i]);

    // Send to client
    network->Send(std::make_shared<Message::UpdatePredictors>
        (predictorsMessage), client);
    network->Send(std::make_shared<Message::UpdatePredictors>
        (responsesMessage), client);
    network->Send(std::make_shared<Message::UpdatePredictors>
        (weightsMessage), client);
  }
}

template <typename T>
void DistFunctionServer
::MakeAliasCols(T& out, T& in, size_t startCol, size_t endCol,
                const typename std::enable_if_t<IsVector<T>::value>* = 0)
{
  mlpack::MakeAlias(out, in, endCol - startCol, startCol);
}

template <typename T>
void DistFunctionServer
::MakeAliasCols(T& out, T& in, size_t startCol, size_t endCol,
                const typename std::enable_if_t<IsMatrix<T>::value>* = 0)
{
  mlpack::MakeAlias(out, in, in.n_rows, endCol - startCol,
      startCol * in.n_rows);
}

template <typename T>
void DistFunctionServer
::MakeAliasCols(T& out, T& in, size_t startCol, size_t endCol,
                const typename std::enable_if_t<IsCube<T>::value>* = 0)
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
    aliases.emplace_back();
    MakeAliasCols(aliases.back(), data, begin, begin + nCols);
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
  // Aliases for the area of gradient each client is responsible for
  std::vector<MONS_MAT_TYPE> gradientAliases(clients.size());

  for (size_t i = 0; i < clients.size(); i++)
  {
    // Split batch size into parts
    size_t nCols = (batchSize - beginTemp) / (clients.size() - i);
    Message::EvaluateWithGradient message;
    message.SetTensor(parameters);
    message.EvaluateWithGradientData.begin = begin;
    message.EvaluateWithGradientData.batchSize = nCols;

    // Create future
    responses[i] = network->SendAwaitable(message, clients[i]);
    // Create alias
    mlpack::MakeAlias(gradientAliases[i], gradient, gradient.n_rows, nCols,
        beginTemp * gradient.n_rows);
    beginTemp += nCols + 1;
  }

  // Wait on all futures
  arma::wall_clock clock;
  clock.tic();
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
        responses[i].get().GetTensor(responseGradient);
        gradientAliases[i] = responseGradient;
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
