/*
 * Message telling client to update predictors with the attached data
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_PREDICTORS_HPP
#define MONS_COMMON_MESSAGE_UPDATE_PREDICTORS_HPP

#include "tensor.hpp"

namespace mons {
namespace Common {
namespace Networking {
namespace Message {

class UpdatePredictors : public TensorType
<typename MONS_PREDICTOR_TYPE::elem_type>
{
public:
  using ParentType = TensorType<typename MONS_PREDICTOR_TYPE::elem_type>;
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
    return Networking::Message::MessageTypes::UpdatePredictors;
  };
};

} // namespace Message
} // namespace Networking
} // namespace Common
} // namespace mons

#endif