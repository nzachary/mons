/*
 * Message telling client to update weights with the attached data
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_WEIGHTS_HPP
#define MONS_COMMON_MESSAGE_UPDATE_WEIGHTS_HPP

#include "tensor.hpp"

namespace mons {
namespace Message {

class UpdateWeights : public Tensor
{
public:
protected:
  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::UpdateWeights;
  };
};

} // namespace Message
} // namespace mons

#endif