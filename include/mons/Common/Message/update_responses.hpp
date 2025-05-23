/*
 * Message telling client to update responses with the attached data
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_RESPONSES_HPP
#define MONS_COMMON_MESSAGE_UPDATE_RESPONSES_HPP

#include "tensor.hpp"

namespace mons {
namespace Message {

class UpdateResponses : public Tensor
{
public:
protected:
  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::UpdateResponses;
  };
};

} // namespace Message
} // namespace mons

#endif