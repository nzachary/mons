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
    uint32_t messageType = 0;

    // Total number of bytes in message; Set in `vector<char> Base::Serialize`
    uint32_t messageNumBytes = 0;
  
    // Sending machine's ID
    uint32_t sender = -1;
  
    // Recieving machine's ID
    uint32_t reciever = -1;
  
    // The identifier of the message
    // Usually only set > 0 if it is expecting a response
    uint32_t id = 0;

    // Mark the message as a response to another message
    // Set it to the original message's ID
    uint32_t responseTo = 0;
  } BaseData;

  // Write message as a byte buffer
  MessageBuffer Serialize()
  {
    MessageBuffer buffer(0);
    Serialize(buffer, true);
    // Overwrite `messageType` and `messageNumBytes`
    mons::Serialize(buffer, MessageType(), true,
        offsetof(BaseDataStruct, BaseDataStruct::messageType));
    mons::Serialize(buffer, (uint32_t)buffer.data.size(), true,
        offsetof(BaseDataStruct, BaseDataStruct::messageNumBytes));
    return buffer;
  };

  // Deserialize from a buffer; returns number of bytes read
  size_t Deserialize(MessageBuffer& buffer)
  {
    Serialize(buffer, false);
    size_t read = buffer.deserializePtr;
    buffer.deserializePtr = 0;
    return read;
  };

  // Serialize or deserialize message metadata
  static void
  SerializeHeader(BaseDataStruct& header, MessageBuffer& buffer, bool direction)
  {
    mons::Serialize(buffer, header.apiVersion, direction);
    // Overwritten in `Serialize` above
    mons::Serialize(buffer, header.messageType, direction);
    // Overwritten in `Serialize` above
    mons::Serialize(buffer, header.messageNumBytes, direction);
    mons::Serialize(buffer, header.sender, direction);
    mons::Serialize(buffer, header.reciever, direction);
    mons::Serialize(buffer, header.id, direction);
    mons::Serialize(buffer, header.responseTo, direction);

    buffer.Expect(sizeof(BaseDataStruct), direction);
  }
protected:
  // Serialize or deserialize message from buffer
  // If `serialize` is true, serialize, else deserialize
  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    // Deserialize is handled in `SerializeHeader` from `Network::Recieve`
    if (serialize)
    {
      SerializeHeader(BaseData, buffer, serialize);
    }
  }
  // Get message type
  virtual uint32_t MessageType() const = 0;
};

} // namespace Message
} // namespace mons

#endif
