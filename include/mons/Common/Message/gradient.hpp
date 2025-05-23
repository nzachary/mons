/*
 * Message to respond to server with gradient
 */
#ifndef MONS_COMMON_MESSAGE_GRADIENT_HPP
#define MONS_COMMON_MESSAGE_GRADIENT_HPP

#include "tensor.hpp"

namespace mons {
namespace Message {

class Gradient : public Tensor
{
public:
struct GradientDataStruct
  {
    MONS_ELEM_TYPE objective;
  } GradientData;
protected:
  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::Gradient;
  }

  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    Tensor::Serialize(buffer, serialize);

    mons::Serialize(buffer, GradientData.objective, serialize);

    buffer.Expect(sizeof(GradientDataStruct), serialize);
  }
};

} // namespace Message
} // namespace mons

#endif