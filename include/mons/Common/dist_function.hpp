/*
 * Networkable version of an optimizable function
 * eg. `mlpacK::FFN`, `mlpack::RNN`, `mlpack::LinearSVM`, etc.
 * See Client/dist_function_client.hpp and Server/dist_function_server.hpp
 */
#ifndef MONS_COMMON_DIST_FUNCTION_HPP
#define MONS_COMMON_DIST_FUNCTION_HPP

#include "../config.hpp"

namespace mons {
namespace Common {

class DistFunction
{
public:
  MONS_FUNCTION_TYPE& GetFunction() { return function; };
protected:
  MONS_FUNCTION_TYPE function;
};

} // namespace Common
} // namespace mons

#endif


