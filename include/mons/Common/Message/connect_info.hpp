/*
 * Information about the local configuration
 */
#ifndef MONS_COMMON_MESSAGE_CONNECT_INFO_HPP
#define MONS_COMMON_MESSAGE_CONNECT_INFO_HPP

#include "base.hpp"
#include "../config_hash.hpp"

namespace mons {
namespace Message {

class ConnectInfo : public Base
{
public:
  struct ConnectInfoDataStruct
  {
    uint64_t config = CONFIG_HASH;
  } ConnectInfoData;
protected:
  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    Base::Serialize(buffer, serialize);

    mons::Serialize(buffer, ConnectInfoData.config, serialize);

    buffer.Expect(sizeof(ConnectInfoDataStruct), serialize);
  }
  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::ConnectInfo;
  }
};

} // namespace Message
} // namespace mons

#endif
