/*
 * Message telling client to update predictors with the attached data
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_PREDICTORS_MESSAGE_HPP
#define MONS_COMMON_MESSAGE_UPDATE_PREDICTORS_MESSAGE_HPP

#include "tensor_message.hpp"


namespace mons {
namespace Common {
namespace Networking {
namespace Message {

class UpdatePredictorsMessage : public TensorMessageType
<typename MONS_PREDICTOR_TYPE::elem_type>
{
protected:
  virtual uint32_t MessageType() const
  {
    return Networking::Message::MessageTypes::UpdatePredictorsMessage;
  };
};

} // namespace Message
} // namespace Networking
} // namespace Common
} // namespace mons

#endif