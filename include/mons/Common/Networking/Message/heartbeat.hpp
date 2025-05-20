/*
 * Message letting the reciever know that the sender is still online
 */
#ifndef MONS_COMMON_MESSAGE_HEARTBEAT_HPP
#define MONS_COMMON_MESSAGE_HEARTBEAT_HPP

#include "base.hpp"

namespace mons {
namespace Common {
namespace Networking {
namespace Message {

class Heartbeat : public Base
{
public:
  struct HeartbeatDataStruct
  {
    // Number that is incremented every time this is sent
    uint32_t beatCount;
  } HeartbeatData;
protected:
  virtual void Serialize(std::vector<char>& buffer)
  {
    Base::Serialize(buffer);

    mons::Common::Serialize<uint32_t>(buffer,
        HeartbeatData.beatCount);
  };

  virtual void Deserialize(std::vector<char>& buffer, size_t& begin)
  {
    Base::Deserialize(buffer, begin);

    mons::Common::Deserialize<uint32_t>(buffer,
        HeartbeatData.beatCount, begin);
  };

  virtual uint32_t MessageType() const
  {
    return Networking::Message::MessageTypes::Heartbeat;
  };
};

} // namespace Message
} // namespace Networking
} // namespace Common
} // namespace mons

#endif
