/*
 * Distributed version of a function that can be optimized by ensmallen
 * optimizers eg. `mlpacK::FFN`, `mlpack::RNN`, `mlpack::LinearSVM`, etc.
 * This is an abstract class
 * See Client/dist_function_client.hpp and Server/dist_function_server.hpp
 */
#ifndef MONS_COMMON_DIST_FUNCTION_HPP
#define MONS_COMMON_DIST_FUNCTION_HPP

#include "function_accessor.hpp"
#include "network.hpp"

namespace mons {

class DistFunction
{
public:
  template <typename... Args>
  DistFunction(Args&&... args) : function(args...) {}
  // Get the underlying function
  MONS_FUNCTION_TYPE& GetFunction() { return function.Get(); }
protected:
  // Function this is wrapped around
  FunctionAccessor function;
};

} // namespace mons

#endif


