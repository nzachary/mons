/*
 * Message telling client to evaluate with a gradient
 */
#ifndef MONS_COMMON_MESSAGE_EVALUATE_WITH_GRADIENT_HPP
#define MONS_COMMON_MESSAGE_EVALUATE_WITH_GRADIENT_HPP

#include "tensor.hpp"

namespace mons {
namespace Message {

class EvaluateWithGradient : public Tensor
{
public:
  struct EvaluateWithGradientDataStruct
  {
    uint64_t begin;
    uint64_t batchSize;
  } EvaluateWithGradientData;
protected:
  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    Tensor::Serialize(buffer, serialize);

    mons::Serialize(buffer, EvaluateWithGradientData.begin, serialize);
    mons::Serialize(buffer, EvaluateWithGradientData.batchSize, serialize);

    buffer.Expect(sizeof(EvaluateWithGradientDataStruct), serialize);
  }

  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::EvaluateWithGradient;
  };
};

} // namespace Message
} // namespace mons

#endif