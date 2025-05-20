/*
 * See Common/dist_function.hpp
 */
#ifndef MONS_CLIENT_DIST_FUNCTION_CLIENT_HPP
#define MONS_CLIENT_DIST_FUNCTION_CLIENT_HPP

#include <mlpack.hpp>

#include "../common.hpp"

namespace mons {
namespace Client {

class DistFunctionClient : public mons::DistFunction
{
public:
  DistFunctionClient(mons::Network& network);
private:
  // Remotely callable functions
  MONS_ELEM_TYPE
  EvaluateWithGradient(const MONS_MAT_TYPE& parameters,
                       const size_t begin,
                       MONS_MAT_TYPE& gradient,
                       const size_t batchSize);

  void Shuffle();

  // Local copies of data subsets

  // Set of input data points.
  MONS_PREDICTOR_TYPE predictors;

  // Set of responses to the input data points.
  MONS_RESPONSE_TYPE responses;

  // Set of weights to the input data points.
  MONS_WEIGHT_TYPE weights;
};

} // namespace Server
} // namespace mons

#include "dist_function_client_impl.hpp"

#endif
