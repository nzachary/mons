/*
 * Message telling the client to shuffle data
 */
#ifndef MONS_COMMON_MESSAGE_SHUFFLE_MESSAGE_HPP
#define MONS_COMMON_MESSAGE_SHUFFLE_MESSAGE_HPP

#include "network_message.hpp"

namespace mons {
namespace Common {
namespace Networking {
namespace Message {

class ShuffleMessage : public NetworkMessage
{
protected:
  virtual uint32_t MessageType() const
  {
    return Networking::Message::MessageTypes::ShuffleMessage;
  };
};

} // namespace Message
} // namespace Networking
} // namespace Common
} // namespace mons

#endif
