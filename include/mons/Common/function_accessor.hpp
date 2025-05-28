/*
 * Helper class to assist with accessing mlpack's internal members
 */
#ifndef MONS_CLIENT_FUNCTION_ACCESSOR_HPP
#define MONS_CLIENT_FUNCTION_ACCESSOR_HPP

#include <mlpack.hpp>

#include "../config.hpp"
#include "../Common/log.hpp"
#include "../Common/has_train_form.hpp"

#define Q(v) #v
#define QQ(v) Q(v)

namespace mons {
namespace Private {

// Optimizer that returns immediately
class EmptyOptimizer
{
public:
  template<typename SeparableFunctionType,
           typename MatType,
           typename... CallbackTypes>
  MONS_ELEM_TYPE Optimize(SeparableFunctionType& /*function*/,
                          MatType& /*iterate */,
                          CallbackTypes&&... /*callbacks*/)
  {
    return 0;
  }
};

} // namespace Private

class FunctionAccessor
{
public:
  template <typename... Args>
  FunctionAccessor(Args&&... args) : function(args...) {}
  // Get the underlying function
  MONS_FUNCTION_TYPE& Get() { return function; }
  const MONS_FUNCTION_TYPE& Get() const { return function; }

  // Initalize the function outside of training
  // Generic initalizer
  void Initalize()
  {
    // Start training but use an empty initalizer that returns immediately
    // so that nothing is actually done other than initalizing
    _Initalize(function);
  }

  void SetPredictors(MONS_PREDICTOR_TYPE predictors) {
    this->predictors = std::move(predictors);
  };

  void SetResponses(MONS_RESPONSE_TYPE responses) {
    this->responses = std::move(responses);
  };

  void SetWeights(MONS_WEIGHT_TYPE weights) {
    this->weights = std::move(weights);
  };

  // Sets input dimensions - internal use
  template <typename T>
  void _SetInputDims(const std::vector<T>& dims)
  {
    inputDims.resize(dims.size());
    for (size_t i = 0; i < dims.size(); i++)
      inputDims[i] = dims[i];
  }
private:
  // Initalizer using only predictors and responses
  template <typename FuncType>
  void _Initalize(FuncType& func,
                  const typename std::enable_if_t<
                    UseTrain2<FuncType>::value>* = 0)
  {
    Private::EmptyOptimizer opt;
    func.Train(std::move(predictors), std::move(responses), opt);
  }

  // Initalizer using only predictors, responses, and weights
  template <typename FuncType>
  void _Initalize(FuncType& func,
                  const typename std::enable_if_t<
                    UseTrain3<FuncType>::value>* = 0)
  {
    Private::EmptyOptimizer opt;
    func.Train(std::move(predictors), std::move(responses),
        std::move(weights), opt);
  }

  // Actual function
  MONS_FUNCTION_TYPE function;

  // Input dimensions
  std::vector<size_t> inputDims;
  
  // Temporary storage for data until the function is initalized
  MONS_PREDICTOR_TYPE predictors;
  MONS_RESPONSE_TYPE responses;
  MONS_WEIGHT_TYPE weights;

  friend class DistFunctionClient;
};

} // namespace mons

#undef QQ
#undef Q

#endif
