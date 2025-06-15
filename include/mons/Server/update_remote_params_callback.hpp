/*
 * Callback to update model parameters on the remote clients
 */
#ifndef MONS_SERVER_UPDATE_REMOTE_PARAMS_CALLBACK_HPP
#define MONS_SERVER_UPDATE_REMOTE_PARAMS_CALLBACK_HPP

#include "async_client_iterator.hpp"
#include "../common.hpp"

namespace mons {
namespace Server {

class UpdateRemoteParamsCallback
{
public:
  UpdateRemoteParamsCallback(AsyncClientIterator& clientIterator)
  : clientIterator(clientIterator)
  {
  }

  template<typename OptimizerType, typename FunctionType, typename MatType>
  void StepTaken(OptimizerType& optimizer,
                 FunctionType& function,
                 MatType& /* coordinates */)
  {
    Message::UpdateParameters message;
    Message::Tensor::SetTensor(function.Parameters(), message.TensorData);
    clientIterator.get().Iterate([&](RemoteClient& client, size_t /* i */)
    {
      if (client.SendOpWait(message) != 0)
        Log::Error("Error while setting parameters");
    });
  }
private:
  std::reference_wrapper<AsyncClientIterator> clientIterator;
};

} // namespace Server
} // namespace mons

#endif
