/*
 * Message containing the status of a generic remote operation
 */
#ifndef MONS_COMMON_MESSAGE_OPERATION_STATUS_HPP
#define MONS_COMMON_MESSAGE_OPERATION_STATUS_HPP

#include "base.hpp"

namespace mons {
namespace Message {

class OperationStatus : public Base
{
public:
  struct OperationStatusStruct
  {
    // Integer representing status of an operation
    // This can be an error code or something else appropriate to the original operation
    int16_t status = 0;
  } OperationStatusData;
protected:
  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    Base::Serialize(buffer, serialize);

    mons::Serialize(buffer, OperationStatusData.status, serialize);

    buffer.Expect(sizeof(OperationStatusStruct), serialize);
  };

  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::OperationStatus;
  };
};

} // namespace Message
} // namespace mons

#endif
