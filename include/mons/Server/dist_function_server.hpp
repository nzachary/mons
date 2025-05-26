/*
 * See Common/dist_function.hpp
 */
#ifndef MONS_SERVER_DIST_FUNCTION_SERVER_HPP
#define MONS_SERVER_DIST_FUNCTION_SERVER_HPP

#include "../common.hpp"

namespace mons {
namespace Server {

class DistFunctionServer : public DistFunction
{
public:
  DistFunctionServer(mons::Network& network);
  // Training without an optimizer
  template<typename OptimizerType, typename... CallbackTypes>
  MONS_ELEM_TYPE Train(MONS_PREDICTOR_TYPE predictors,
                       MONS_RESPONSE_TYPE responses,
                       CallbackTypes&&... callbacks);
  // Training with an optimizer
  template<typename OptimizerType, typename... CallbackTypes>
  MONS_ELEM_TYPE Train(MONS_PREDICTOR_TYPE predictors,
                       MONS_RESPONSE_TYPE responses,
                       OptimizerType& optimizer,
                       CallbackTypes&&... callbacks);
  // Training with weights and no optimizer
  template<typename OptimizerType, typename... CallbackTypes>
  MONS_ELEM_TYPE Train(MONS_PREDICTOR_TYPE predictors,
                       MONS_RESPONSE_TYPE responses,
                       MONS_WEIGHT_TYPE weights,
                       CallbackTypes&&... callbacks);
  // Training with an optimizer and weights
  template<typename OptimizerType, typename... CallbackTypes>
  MONS_ELEM_TYPE Train(MONS_PREDICTOR_TYPE predictors,
                       MONS_RESPONSE_TYPE responses,
                       MONS_WEIGHT_TYPE weights,
                       OptimizerType& optimizer,
                       CallbackTypes&&... callbacks);

  // Functions required for optimizer
  MONS_ELEM_TYPE
  EvaluateWithGradient(const MONS_MAT_TYPE& parameters,
                       const size_t begin,
                       MONS_MAT_TYPE& gradient,
                       const size_t batchSize);
  
  size_t NumFunctions() const { return function.Get().NumFunctions(); }
  
  void Shuffle();

  MONS_MAT_TYPE& Parameters() { return function.Get().Parameters(); }
private:
  // Utility functions
  // Set predictors, responses, and weights on a remote client
  void SendData(MONS_PREDICTOR_TYPE& predictors,
                 MONS_RESPONSE_TYPE& responses,
                 MONS_WEIGHT_TYPE& weights);

  // List of known clients
  std::vector<std::reference_wrapper<RemoteClient>> clients;

  // List of clients that have been initalized
  std::vector<std::reference_wrapper<RemoteClient>> initalizedClients;
};

} // namespace Server
} // namespace mons

#include "dist_function_server_impl.hpp"

#endif
