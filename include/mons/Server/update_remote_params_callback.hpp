/*
 * Callback to update model parameters on the remote clients
 */
#ifndef MONS_SERVER_UPDATE_REMOTE_PARAMS_CALLBACK_HPP
#define MONS_SERVER_UPDATE_REMOTE_PARAMS_CALLBACK_HPP

#include "../common.hpp"

namespace mons {
namespace Server {

class UpdateRemoteParamsCallback
{
public:
  UpdateRemoteParamsCallback(std::vector<std::reference_wrapper<RemoteClient>>& clients)
  : clients(clients)
  {
  }

  template<typename OptimizerType, typename FunctionType, typename MatType>
  void StepTaken(OptimizerType& optimizer,
                 FunctionType& function,
                 MatType& /* coordinates */)
  {
    Message::UpdateParameters message;
    Message::Tensor::SetTensor(function.Parameters(), message.TensorData);
    for (RemoteClient& client : clients)
    {
      if (client.SendOpWait(message) != 0)
        Log::Error("Error while setting parameters");
    }
  }
private:
  std::vector<std::reference_wrapper<RemoteClient>> clients;
};

} // namespace Server
} // namespace mons

#endif
