/*
 * Message telling client to evaluate with a gradient
 */
#ifndef MONS_COMMON_MESSAGE_EVALUATE_WITH_GRADIENT_HPP
#define MONS_COMMON_MESSAGE_EVALUATE_WITH_GRADIENT_HPP

#include "tensor.hpp"

namespace mons {
namespace Message {

class EvaluateWithGradient : public TensorType
<typename MONS_MAT_TYPE::elem_type>
{
public:
  using ParentType = TensorType<typename MONS_MAT_TYPE::elem_type>;
  struct EvaluateWithGradientDataStruct
  {
    uint64_t begin;
    uint64_t batchSize;
  } EvaluateWithGradientData;
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
  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    ParentType::Serialize(buffer, serialize);

    mons::Serialize(buffer, EvaluateWithGradientData.begin, serialize);
    mons::Serialize(buffer, EvaluateWithGradientData.batchSize, serialize);

    buffer.Expect(sizeof(EvaluateWithGradientDataStruct), serialize);
  }

  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::EvaluateWithGradient;
  };
};

} // namespace Message
} // namespace mons

#endif