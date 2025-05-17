/*
 * Message telling client to update responses with the attached data
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_RESPONSES_MESSAGE_HPP
#define MONS_COMMON_MESSAGE_UPDATE_RESPONSES_MESSAGE_HPP

#include "tensor_message.hpp"


namespace mons {
namespace Common {
namespace Networking {
namespace Message {

class UpdateResponsesMessage : public TensorMessageType
<typename MONS_RESPONSE_TYPE::elem_type>
{
protected:
  virtual uint32_t MessageType() const
  {
    return Networking::Message::MessageTypes::UpdateResponsesMessage;
  };
};

} // namespace Message
} // namespace Networking
} // namespace Common
} // namespace mons

#endif