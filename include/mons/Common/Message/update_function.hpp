/*
 * Initalize the model of a function on a client from server
 */
#ifndef MONS_COMMON_MESSAGE_UPDATE_FUNCTION_HPP
#define MONS_COMMON_MESSAGE_UPDATE_FUNCTION_HPP

#include "cereal.hpp"

namespace mons {
namespace Message {
  
class UpdateFunction : public Cereal
{
public:
  struct UpdateFunctionDataStruct {
    // Number of elements in `inputDimensions`
    uint16_t numDims;
    // Lengths of each dimension
    std::vector<uint16_t> inputDimensions;
  } UpdateFunctionData;

  // Sets the message's input dimensions
  void SetInputDimension(std::vector<size_t>& dimensions)
  {
    UpdateFunctionData.numDims = dimensions.size();
    UpdateFunctionData.inputDimensions.resize(dimensions.size());
    for (size_t i = 0; i < dimensions.size(); i++)
      UpdateFunctionData.inputDimensions[i] = dimensions[i];
  }
protected:
  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    Cereal::Serialize(buffer, serialize);

    mons::Serialize(buffer, UpdateFunctionData.numDims, serialize);
    if (!serialize)
      UpdateFunctionData.inputDimensions.resize(UpdateFunctionData.numDims);
    mons::Serialize(buffer, UpdateFunctionData.inputDimensions, serialize);

    buffer.Expect(sizeof(UpdateFunctionData.numDims) +
        UpdateFunctionData.numDims * sizeof(uint16_t), serialize);
  }
  virtual uint32_t MessageType() const
  {
    return Message::MessageTypes::UpdateFunction;
  }
};

} // namespace Message
} // namespace mons

#endif
