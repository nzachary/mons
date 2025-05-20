/*
 * Message letting the reciever know that the sender is still online
 */
#ifndef MONS_COMMON_MESSAGE_HEARTBEAT_HPP
#define MONS_COMMON_MESSAGE_HEARTBEAT_HPP

#include "base.hpp"

namespace mons {
namespace Message {

class Heartbeat : public Base
{
public:
  struct HeartbeatDataStruct
  {
    // Number that is incremented every time this is sent
    uint32_t beatCount = 0;
  } HeartbeatData;
protected:
  virtual void Serialize(std::vector<char>& buffer)
  {
    Base::Serialize(buffer);

    mons::Serialize(buffer, HeartbeatData.beatCount);
  };

  virtual void Deserialize(std::vector<char>& buffer, size_t& begin)
  {
    Base::Deserialize(buffer, begin);

    mons::Deserialize(buffer, HeartbeatData.beatCount, begin);
  };

  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::Heartbeat;
  };
};

} // namespace Message
} // namespace mons

#endif
