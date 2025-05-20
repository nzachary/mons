#ifndef MONS_COMMON_MESSAGE_BASE_HPP
#define MONS_COMMON_MESSAGE_BASE_HPP

#include <vector>
#include <stdint.h>

#include "../../version.hpp"
#include "../serialize.hpp"
#include "../log.hpp"
#include "register_types.hpp"

namespace mons {
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

class Base
{
public:
  struct BaseDataStruct
  {
    // Version of api used when creating this
    uint32_t apiVersion = MONS_VERSION_NUMBER;

    // Type ID of message being sent; Set in `vector<char> Base::Serialize`
    uint32_t messageType = -1;

    // Total number of bytes in message; Set in `vector<char> Base::Serialize`
    uint32_t messageNumBytes = 0;
  
    // Sending machine's ID
    uint32_t sender = 0;
  
    // Recieving machine's ID
    uint32_t reciever = 0;
  
    // ID is equal to the number of messages the sender has sent out
    uint32_t id = 0;

    // Mark the message as a response to another message
    // Set it to the original message's ID
    uint32_t responseTo = 0;
  } BaseData;

  // Write message as a byte buffer
  std::vector<char> Serialize()
  {
    std::vector<char> buffer;
    Serialize(buffer);
    // Overwrite `messageType` and `messageNumBytes`
    mons::Serialize(buffer, MessageType(), offsetof(BaseDataStruct,
        BaseDataStruct::messageType));
    mons::Serialize(buffer, buffer.size(), offsetof(BaseDataStruct,
        BaseDataStruct::messageNumBytes));
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
  static BaseDataStruct
  DecodeHeader(std::vector<char>& buffer)
  {
    assert(buffer.size() == sizeof(BaseDataStruct));
    size_t begin = 0;
    BaseDataStruct tempStruct;

    mons::Deserialize(buffer, tempStruct.apiVersion, begin);
    mons::Deserialize(buffer, tempStruct.messageType, begin);
    mons::Deserialize(buffer, tempStruct.messageNumBytes, begin);
    mons::Deserialize(buffer, tempStruct.sender, begin);
    mons::Deserialize(buffer, tempStruct.reciever, begin);
    mons::Deserialize(buffer, tempStruct.id, begin);
    mons::Deserialize(buffer, tempStruct.responseTo, begin);
    
    assert(begin == sizeof(BaseDataStruct));

    return tempStruct;
  }
protected:
  // Append data to a byte buffer
  virtual void Serialize(std::vector<char>& buffer)
  {
    mons::Serialize(buffer, BaseData.apiVersion);
    // Dummy - see `Serialize` above
    mons::Serialize(buffer, BaseData.messageType);
    // Dummy - see `Serialize` above
    mons::Serialize(buffer, BaseData.messageNumBytes);
    mons::Serialize(buffer, BaseData.sender);
    mons::Serialize(buffer, BaseData.reciever);
    mons::Serialize(buffer, BaseData.id);
    mons::Serialize(buffer, BaseData.responseTo);
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
} // namespace mons

#endif
