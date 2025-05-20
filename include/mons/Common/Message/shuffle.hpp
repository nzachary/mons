/*
 * Message telling the client to shuffle data
 */
#ifndef MONS_COMMON_MESSAGE_SHUFFLE_HPP
#define MONS_COMMON_MESSAGE_SHUFFLE_HPP

#include "base.hpp"

namespace mons {
namespace Message {

class Shuffle : public Base
{
protected:
  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::Shuffle;
  };
};

} // namespace Message
} // namespace mons

#endif
