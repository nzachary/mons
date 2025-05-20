/*
 * Message to respond to server with gradient
 */
#ifndef MONS_COMMON_MESSAGE_GRADIENT_HPP
#define MONS_COMMON_MESSAGE_GRADIENT_HPP

#include "tensor.hpp"

namespace mons {
namespace Message {

class Gradient : public TensorType
<typename MONS_MAT_TYPE::elem_type>
{
public:
  using ParentType = TensorType<typename MONS_MAT_TYPE::elem_type>;
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
    return Message::MessageTypes::Gradient;
  };
};

} // namespace Message
} // namespace mons

#endif