#ifndef MONS_SERVER_DIST_FUNCTION_SERVER_IMPL_HPP
#define MONS_SERVER_DIST_FUNCTION_SERVER_IMPL_HPP

#include "dist_function_server.hpp"
#include "../Common/Networking/network.hpp"

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
  // Split parameters into parts and send them off to clients
  std::vector<MONS_PREDICTOR_TYPE> predictorAliases =
      Split(predictors, 1);
  std::vector<MONS_RESPONSE_TYPE> responseAliases =
      Split(responses, 1);
  std::vector<MONS_WEIGHT_TYPE> weightAliases =
      Split(weights, 1);
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
  // Get number of connected clients
}

size_t DistFunctionServer::NumFunctions() const
{
  //return responses.n_cols;
}

void DistFunctionServer::Shuffle()
{
  // Rather than shuffling here, tell worker clients to shuffle their data
}

} // namespace Server
} // namespace mons

#endif
