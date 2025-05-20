/*
 * See Common/dist_function.hpp
 */
#ifndef MONS_SERVER_DIST_FUNCTION_SERVER_HPP
#define MONS_SERVER_DIST_FUNCTION_SERVER_HPP

#include "../common.hpp"

namespace mons {
namespace Server {

class DistFunctionServer : public mons::DistFunction
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
private:
  // Utility functions
  // Set predictors, responses, and weights
  void ResetData(MONS_PREDICTOR_TYPE predictors,
                 MONS_RESPONSE_TYPE responses,
                 MONS_WEIGHT_TYPE weights = MONS_WEIGHT_TYPE());
  
  // Attempts to make an alias with specified cols
  template <typename T>
  void MakeAliasCols(T& out, T& in, size_t startCol, size_t endCol,
                     const typename std::enable_if_t<IsVector<T>::value>* = 0);
  template <typename T>
  void MakeAliasCols(T& out, T& in, size_t startCol, size_t endCol,
                     const typename std::enable_if_t<IsMatrix<T>::value>* = 0);
  template <typename T>
  void MakeAliasCols(T& out, T& in, size_t startCol, size_t endCol,
                     const typename std::enable_if_t<IsCube<T>::value>* = 0);

  // Splits `data` into `n` aliases, split over columns
  template <typename DataType>
  std::vector<DataType> Split(DataType& data, size_t n);

  // Functions required for optimizer
  MONS_ELEM_TYPE
  EvaluateWithGradient(const MONS_MAT_TYPE& parameters,
                       const size_t begin,
                       MONS_MAT_TYPE& gradient,
                       const size_t batchSize);

  size_t NumFunctions() const;

  void Shuffle();

  // List of connected clients
  std::vector<id_t> clients = { 1 };

  // Number of functions since we don't hold on to training data
  size_t numFunctions;
};

} // namespace Server
} // namespace mons

#endif
