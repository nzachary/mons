/*
 * Message containing a dense tensor
 */
#ifndef MONS_COMMON_MESSAGE_TENSOR_MESSAGE_HPP
#define MONS_COMMON_MESSAGE_TENSOR_MESSAGE_HPP

#include <armadillo>
#include <mlpack/core/util/arma_traits.hpp>

#include "../../../config.hpp"
#include "network_message.hpp"

namespace mons {
namespace Common {
namespace Networking {
namespace Message {

template <typename eT>
class TensorMessageType : public NetworkMessage
{
public:
  struct TensorMessageDataStruct
  {
    // Number of dimensions
    uint16_t numDimensions;
    // Lengths of each dimension
    std::vector<uint64_t> dimensions;
    // Element data
    std::vector<eT> data;
  } TensorMessageData;

  // Checks if the contained tensor is 1, 2, or 3 dimensional
  bool TensorIsVector() const { return TensorMessageData.numDimensions == 1; };
  bool TensorIsMatrix() const { return TensorMessageData.numDimensions == 2; };
  bool TensorIsCube() const { return TensorMessageData.numDimensions == 3; };

  // TODO: bandicoot types
  // Gets the contained tensor as type T
  template <typename T>
  void GetTensor(T& tensor,
                 const typename std::enable_if_t<
                   IsVector<T>::value &&
                   std::is_same<typename T::elem_type, eT>::value
                 >* = 0) const
  {
    if (!TensorIsVector())
    {
      Log::Error("Trying to get tensor as row but dimensionality is " +
          std::to_string(TensorMessageData.numDimensions));
      new (&tensor) T();
      return;
    }
    
    // Placement new to avoid copy operation
    new (&tensor) T(TensorMessageData.data.data(),
        TensorMessageData.dimensions[0]);
  }

  template <typename T>
  void GetTensor(T& tensor,
                 const typename std::enable_if_t<
                   IsMatrix<T>::value &&
                   std::is_same<typename T::elem_type, eT>::value
                 >* = 0) const
  {
    if (!TensorIsMatrix())
    {
      Log::Error("Trying to get tensor as matrix but dimensionality is " +
          std::to_string(TensorMessageData.numDimensions));
      new (&tensor) T();
      return;
    }
    
    // Placement new to avoid copy operation
    new (&tensor) T(TensorMessageData.data.data(),
        TensorMessageData.dimensions[0], TensorMessageData.dimensions[1]);
  }

  template <typename T>
  void GetTensor(T& tensor,
                 const typename std::enable_if_t<
                   IsCube<T>::value &&
                   std::is_same<typename T::elem_type, eT>::value
                 >* = 0) const
  {
    if (!TensorIsCube())
    {
      Log::Error("Trying to get tensor as cube but dimensionality is " +
          std::to_string(TensorMessageData.numDimensions));
      new (&tensor) T();
      return;
    }
    
    // Placement new to avoid copy operation
    new (&tensor) T(TensorMessageData.data.data(),
        TensorMessageData.dimensions[0], TensorMessageData.dimensions[1],
        TensorMessageData.dimensions[1]);
  }

  // Sets the tensor to be sent
  template <typename T>
  void SetTensor(T& tensor,
                 const typename std::enable_if_t<
                   IsVector<T>::value &&
                   std::is_same<typename T::elem_type, eT>::value
                 >* = 0)
  {
    TensorMessageData.numDimensions = 1;
    TensorMessageData.dimensions = { tensor.n_elem };
    TensorMessageData.data.resize(tensor.n_elem);
    std::memcpy(TensorMessageData.data.data(), tensor.memptr(),
        tensor.n_elem * sizeof(eT));
  }

  template <typename T>
  void SetTensor(T& tensor,
                 const typename std::enable_if_t<
                   IsMatrix<T>::value &&
                   std::is_same<typename T::elem_type, eT>::value
                 >* = 0)
  {
    TensorMessageData.numDimensions = 2;
    TensorMessageData.dimensions = { tensor.n_rows, tensor.n_cols };
    TensorMessageData.data.resize(tensor.n_elem);
    std::memcpy(TensorMessageData.data.data(), tensor.memptr(),
        tensor.n_elem * sizeof(eT));
  }

  template <typename T>
  void SetTensor(T& tensor,
                 const typename std::enable_if_t<
                   IsCube<T>::value &&
                   std::is_same<typename T::elem_type, eT>::value
                 >* = 0)
  {
    TensorMessageData.numDimensions = 3;
    TensorMessageData.dimensions = { tensor.n_rows, tensor.n_cols,
        tensor.n_slices};
    TensorMessageData.data.resize(tensor.n_elem);
    std::memcpy(TensorMessageData.data.data(), tensor.memptr(),
        tensor.n_elem * sizeof(eT));
  }
protected:
  virtual void Serialize(std::vector<char>& buffer)
  {
    NetworkMessage::Serialize(buffer);

    // Verify data is correctly formatted
    if (TensorMessageData.numDimensions != TensorMessageData.dimensions.size())
    {
      Log::FatalError("Incorrect number of dimensions in tensor");
    }
    size_t numElem = 1;
    for (const uint64_t& len : TensorMessageData.dimensions)
      numElem *= len;
    if (numElem != TensorMessageData.data.size())
    {
      Log::FatalError("Incorrect number of elements in tensor");
    }

    mons::Common::Serialize<uint16_t>(buffer,
        TensorMessageData.numDimensions);
    mons::Common::Serialize<std::vector<uint64_t>>(buffer,
        TensorMessageData.dimensions);
    mons::Common::Serialize<std::vector<eT>>(buffer,
        TensorMessageData.data);
  };

  virtual void Deserialize(std::vector<char>& buffer, size_t& begin)
  {
    NetworkMessage::Deserialize(buffer, begin);

    mons::Common::Deserialize<uint16_t>(buffer,
        TensorMessageData.numDimensions, begin);
    TensorMessageData.dimensions.resize(TensorMessageData.numDimensions);
    mons::Common::Deserialize<std::vector<uint64_t>>(buffer,
        TensorMessageData.dimensions, begin);
    size_t numElem = 1;
    for (const uint64_t& len : TensorMessageData.dimensions)
      numElem *= len;
    TensorMessageData.data.resize(numElem);
    mons::Common::Deserialize<std::vector<eT>>(buffer,
        TensorMessageData.data, begin);
  };
};

} // namespace Message
} // namespace Networking
} // namespace Common
} // namespace mons

#endif
