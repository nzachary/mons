/*
 * Message telling client to update function parameters with the attached data
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_PARAMETERS_HPP
#define MONS_COMMON_MESSAGE_UPDATE_PARAMETERS_HPP

#include "tensor.hpp"

namespace mons {
namespace Message {

class UpdateParameters : public Tensor
{
public:
protected:
  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::UpdateParameters;
  };
};

} // namespace Message
} // namespace mons

#endif