/*
 * Message letting the reciever know that the sender is still online
 */
#ifndef MONS_COMMON_MESSAGE_HEARTBEAT_MESSAGE_HPP
#define MONS_COMMON_MESSAGE_HEARTBEAT_MESSAGE_HPP

#include "network_message.hpp"

namespace mons {
namespace Common {
namespace Networking {
namespace Message {

class HeartbeatMessage : public NetworkMessage
{
public:
  struct HeartbeatMessageDataStruct
  {
    // Number that is incremented every time this is sent
    uint32_t beatCount;
  } HeartbeatMessageData;
protected:
  virtual void Serialize(std::vector<char>& buffer)
  {
    NetworkMessage::Serialize(buffer);

    mons::Common::Serialize<uint32_t>(buffer,
        HeartbeatMessageData.beatCount);
  };

  virtual void Deserialize(std::vector<char>& buffer, size_t& begin)
  {
    NetworkMessage::Deserialize(buffer, begin);

    mons::Common::Deserialize<uint32_t>(buffer,
        HeartbeatMessageData.beatCount, begin);
  };

  virtual uint32_t MessageType() const
  {
    return Networking::Message::MessageTypes::HeartbeatMessage;
  };
};

} // namespace Message
} // namespace Networking
} // namespace Common
} // namespace mons

#endif
