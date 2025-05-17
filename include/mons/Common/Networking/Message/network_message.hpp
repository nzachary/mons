#ifndef MONS_COMMON_MESSAGE_NETWORK_MESSAGE_HPP
#define MONS_COMMON_MESSAGE_NETWORK_MESSAGE_HPP

#include <vector>
#include <stdint.h>

#include "../../../version.hpp"
#include "../../serialize.hpp"
#include "../../log.hpp"
#include "register_types.hpp"

namespace mons {
namespace Common {
namespace Networking {
namespace Message {

// Abstract class for messages that can be sent/recieved over network, do not try to send this
// Children should call the parent's `Serialize` and `Deserialize` before doing their own
// But they should add to `messageNumBytes` before calling `Serialize`

namespace MessageTypes { // Seperate namespace to prevent MessageType from overlapping with message types

// Register message type numbers
#define REGISTER(Val) Val,
enum MessageType
{
  MONS_REGISTER_MESSAGE_TYPES
};
#undef REGISTER

} // namespace MessageTypes

class NetworkMessage
{
public:
  struct NetworkMessageDataStruct
  {
    // Version of api used when creating this
    uint32_t apiVersion = MONS_VERSION_NUMBER;

    // Type ID of message being sent; Dummy
    uint32_t messageType = -1;

    // Total number of bytes in message; Dummy
    uint32_t messageNumBytes = 0;
  
    // Sending machine's ID
    uint32_t sender = 0;
  
    // Recieving machine's ID
    uint32_t reciever = 0;
  } NetworkMessageData;

  // Write message as a byte buffer
  std::vector<char> Serialize()
  {
    std::vector<char> buffer;
    Serialize(buffer);
    mons::Common::Serialize<uint64_t>(buffer, MessageType(),
        offsetof(NetworkMessageDataStruct,
        NetworkMessageDataStruct::messageType));
    mons::Common::Serialize<uint64_t>(buffer, buffer.size(),
        offsetof(NetworkMessageDataStruct,
        NetworkMessageDataStruct::messageNumBytes));
    return buffer;
  };

  // Deserialize from a buffer; returns number of bytes read
  size_t Deserialize(std::vector<char>& buffer)
  {
    size_t begin = 0;
    Deserialize(buffer, begin);
    return begin;
  };

  // Decode message metadata from a buffer
  static NetworkMessageDataStruct
  DecodeHeader(std::vector<char>& buffer)
  {
    assert(buffer.size() == sizeof(NetworkMessageDataStruct));
    size_t begin = 0;
    NetworkMessageDataStruct tempStruct;

    mons::Common::Deserialize<uint32_t>(buffer,
        tempStruct.apiVersion, begin);
    mons::Common::Deserialize<uint32_t>(buffer,
        tempStruct.messageType, begin);
    mons::Common::Deserialize<uint32_t>(buffer,
        tempStruct.messageNumBytes, begin);
    mons::Common::Deserialize<uint32_t>(buffer,
        tempStruct.sender, begin);
    mons::Common::Deserialize<uint32_t>(buffer,
        tempStruct.reciever, begin);

    return tempStruct;
  }
protected:
  // Append data to a byte buffer
  virtual void Serialize(std::vector<char>& buffer)
  {
    mons::Common::Serialize<uint32_t>(buffer,
        NetworkMessageData.apiVersion);
    mons::Common::Serialize<uint32_t>(buffer, // Dummy - see `Serialize` above
        NetworkMessageData.messageType);
    mons::Common::Serialize<uint32_t>(buffer, // Dummy - see `Serialize` above
        NetworkMessageData.messageNumBytes);
    mons::Common::Serialize<uint32_t>(buffer,
        NetworkMessageData.sender);
    mons::Common::Serialize<uint32_t>(buffer,
        NetworkMessageData.reciever);
  };
  // Parse serialized buffer
  virtual void Deserialize(std::vector<char>& buffer, size_t& begin)
  {
    // Handled in `DecodeHeader`
  }
  // Get message type
  virtual uint32_t MessageType() const = 0;
};

} // namespace Message
} // namespace Networking
} // namespace Common
} // namespace mons

#endif
