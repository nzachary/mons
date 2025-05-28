/*
 * See Common/dist_function.hpp
 */
#ifndef MONS_CLIENT_DIST_FUNCTION_CLIENT_HPP
#define MONS_CLIENT_DIST_FUNCTION_CLIENT_HPP

#include <mlpack.hpp>

#include "../common.hpp"

namespace mons {
namespace Client {

class DistFunctionClient : public DistFunction
{
public:
  // Accepts a client that is used to communicate with the main server
  DistFunctionClient(RemoteClient& serverRemote);
private:
  bool isInit = false;
};

} // namespace Server
} // namespace mons

#include "dist_function_client_impl.hpp"

#endif
