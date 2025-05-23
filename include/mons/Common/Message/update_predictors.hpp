/*
 * Message telling client to update predictors with the attached data
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_PREDICTORS_HPP
#define MONS_COMMON_MESSAGE_UPDATE_PREDICTORS_HPP

#include "tensor.hpp"

namespace mons {
namespace Message {

class UpdatePredictors : public Tensor
{
public:
protected:
  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::UpdatePredictors;
  };
};

} // namespace Message
} // namespace mons

#endif