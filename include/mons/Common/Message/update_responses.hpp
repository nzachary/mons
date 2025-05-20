/*
 * Message telling client to update responses with the attached data
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_RESPONSES_HPP
#define MONS_COMMON_MESSAGE_UPDATE_RESPONSES_HPP

#include "tensor.hpp"

namespace mons {
namespace Message {

class UpdateResponses : public TensorType
<typename MONS_RESPONSE_TYPE::elem_type>
{
public:
  using ParentType = TensorType<typename MONS_RESPONSE_TYPE::elem_type>;
  // Wrappers to allow templated functions
  template <typename T>
  void GetTensor(T& tensor) const
  {
    ParentType::GetTensor<T>(tensor);
  };

  template <typename T>
  void SetTensor(T& tensor)
  {
    ParentType::SetTensor<T>(tensor);
  };
protected:
  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::UpdateResponses;
  };
};

} // namespace Message
} // namespace mons

#endif