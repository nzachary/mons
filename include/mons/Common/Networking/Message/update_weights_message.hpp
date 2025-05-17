/*
 * Message telling client to update weights with the attached data
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_WEIGHTS_MESSAGE_HPP
#define MONS_COMMON_MESSAGE_UPDATE_WEIGHTS_MESSAGE_HPP

#include "tensor_message.hpp"


namespace mons {
namespace Common {
namespace Networking {
namespace Message {

class UpdateWeightsMessage : public TensorMessageType
<typename MONS_WEIGHT_TYPE::elem_type>
{
protected:
  virtual uint32_t MessageType() const
  {
    return Networking::Message::MessageTypes::UpdateWeightsMessage;
  };
};

} // namespace Message
} // namespace Networking
} // namespace Common
} // namespace mons

#endif