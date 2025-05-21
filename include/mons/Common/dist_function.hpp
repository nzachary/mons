/*
 * Distributed version of an optimizable function
 * eg. `mlpacK::FFN`, `mlpack::RNN`, `mlpack::LinearSVM`, etc.
 * See Client/dist_function_client.hpp and Server/dist_function_server.hpp
 */
#ifndef MONS_COMMON_DIST_FUNCTION_HPP
#define MONS_COMMON_DIST_FUNCTION_HPP

#include <mlpack.hpp>

#include "../config.hpp"
#include "network.hpp"

namespace mons {

class DistFunction
{
public:
  // Get the underlying function
  MONS_FUNCTION_TYPE& GetFunction() { return function; };
protected:
  // Function this is wrapped around
  MONS_FUNCTION_TYPE function;
};

} // namespace mons

#endif


