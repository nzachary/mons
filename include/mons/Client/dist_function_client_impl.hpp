#ifndef MONS_CLIENT_DIST_FUNCTION_CLIENT_IMPL_HPP
#define MONS_CLIENT_DIST_FUNCTION_CLIENT_IMPL_HPP

#include "dist_function_client.hpp"

namespace mons {
namespace Client {

DistFunctionClient
::DistFunctionClient(RemoteClient& server)
{
  // Register events to update predictors, responses, weights, and function model
  server.OnRecieve([&](const Message::UpdatePredictors& message)
  {
    MONS_PREDICTOR_TYPE predictors;
    Message::Tensor::GetTensor(predictors, message.TensorData);
    function.SetPredictors(predictors);

    // Respond to server
    Message::OperationStatus status;
    status.BaseData.responseTo = message.BaseData.id;
    status.OperationStatusData.status = 0;
    server.Send(status);
    
    return false;
  });
  server.OnRecieve([&](const Message::UpdateResponses& message)
  {
    MONS_RESPONSE_TYPE responses;
    Message::Tensor::GetTensor(responses, message.TensorData);
    function.SetResponses(responses);

    // Respond to server
    Message::OperationStatus status;
    status.BaseData.responseTo = message.BaseData.id;
    status.OperationStatusData.status = 0;
    server.Send(status);

    return false;
  });
  server.OnRecieve([&](const Message::UpdateWeights& message)
  {
    MONS_WEIGHT_TYPE weights;
    Message::Tensor::GetTensor(weights, message.TensorData);
    function.SetWeights(weights);

    // Respond to server
    Message::OperationStatus status;
    status.BaseData.responseTo = message.BaseData.id;
    status.OperationStatusData.status = 0;
    server.Send(status);

    return false;
  });
  server.OnRecieve([&](const Message::UpdateFunction& message)
  {
    Message::OperationStatus status;
    status.BaseData.responseTo = message.BaseData.id;

    auto& data = message.UpdateFunctionData;
    function._SetInputDims(data.inputDimensions);
    if (Message::Cereal::Decerealize(function.Get(), message.CerealData))
    {
      function.Initalize(function.Get());
      status.OperationStatusData.status = 0;
    }
    else
    {
      Log::Error("Failed to deserialize function");
      status.OperationStatusData.status = 1;
    }
    server.Send(status);

    return false;
  });

  // Register functions that can be remotely called
  server.OnRecieve([&](const Message::Shuffle& message)
  {
    Shuffle();

    return false;
  });
  server.OnRecieve([&](const Message::EvaluateWithGradient& message)
  {
    // Evaluate with gradient
    MONS_MAT_TYPE parameters, gradient;
    Message::Tensor::GetTensor(parameters, message.TensorData);

    uint64_t begin, batchSize;
    begin = message.EvaluateWithGradientData.begin;
    batchSize = message.EvaluateWithGradientData.batchSize;

    double obj = EvaluateWithGradient(parameters, begin, gradient, batchSize);

    // Return the result
    Message::Gradient gradientMessage;
    Message::Tensor::SetTensor(gradient, gradientMessage.TensorData);
    gradientMessage.GradientData.objective = obj;
    gradientMessage.BaseData.responseTo = message.BaseData.id;
    
    server.Send(gradientMessage);

    return false;
  });
}

MONS_ELEM_TYPE DistFunctionClient
::EvaluateWithGradient(const MONS_MAT_TYPE& parameters,
                     const size_t begin,
                     MONS_MAT_TYPE& gradient,
                     const size_t batchSize)
{
  return function.Get().EvaluateWithGradient(parameters, begin, gradient, batchSize);
}

void DistFunctionClient::Shuffle()
{
  function.Get().Shuffle();
}

} // namespace Client
} // namespace mons

#endif
