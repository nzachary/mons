/*
 * Message containing a dense tensor
 */
#ifndef MONS_COMMON_MESSAGE_TENSOR_HPP
#define MONS_COMMON_MESSAGE_TENSOR_HPP

#include <armadillo>
#include <mlpack/core/util/arma_traits.hpp>

#include "../../config.hpp"
#include "base.hpp"

namespace mons {
namespace Message {

template <typename eT>
class TensorType : public Base
{
public:
  struct TensorDataStruct
  {
    // Number of dimensions
    uint16_t numDimensions;
    // Lengths of each dimension
    std::vector<uint64_t> dimensions;
    // Element data
    std::vector<eT> data;
  } TensorData;

  // Checks if the contained tensor is 1, 2, or 3 dimensional
  bool TensorIsVector() const { return TensorData.numDimensions == 1; };
  bool TensorIsMatrix() const { return TensorData.numDimensions == 2; };
  bool TensorIsCube() const { return TensorData.numDimensions == 3; };

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
          std::to_string(TensorData.numDimensions));
      new (&tensor) T();
      return;
    }
    
    // Placement new to avoid copy operation
    new (&tensor) T(TensorData.data.data(),
        TensorData.dimensions[0]);
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
          std::to_string(TensorData.numDimensions));
      new (&tensor) T();
      return;
    }
    
    // Placement new to avoid copy operation
    new (&tensor) T(TensorData.data.data(),
        TensorData.dimensions[0], TensorData.dimensions[1]);
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
          std::to_string(TensorData.numDimensions));
      new (&tensor) T();
      return;
    }
    
    // Placement new to avoid copy operation
    new (&tensor) T(TensorData.data.data(),
        TensorData.dimensions[0], TensorData.dimensions[1],
        TensorData.dimensions[1]);
  }

  // Sets the tensor to be sent
  template <typename T>
  void SetTensor(T& tensor,
                 const typename std::enable_if_t<
                   IsVector<T>::value &&
                   std::is_same<typename T::elem_type, eT>::value
                 >* = 0)
  {
    TensorData.numDimensions = 1;
    TensorData.dimensions = { tensor.n_elem };
    TensorData.data.resize(tensor.n_elem);
    std::memcpy(TensorData.data.data(), tensor.memptr(),
        tensor.n_elem * sizeof(eT));
  }

  template <typename T>
  void SetTensor(T& tensor,
                 const typename std::enable_if_t<
                   IsMatrix<T>::value &&
                   std::is_same<typename T::elem_type, eT>::value
                 >* = 0)
  {
    TensorData.numDimensions = 2;
    TensorData.dimensions = { tensor.n_rows, tensor.n_cols };
    TensorData.data.resize(tensor.n_elem);
    std::memcpy(TensorData.data.data(), tensor.memptr(),
        tensor.n_elem * sizeof(eT));
  }

  template <typename T>
  void SetTensor(T& tensor,
                 const typename std::enable_if_t<
                   IsCube<T>::value &&
                   std::is_same<typename T::elem_type, eT>::value
                 >* = 0)
  {
    TensorData.numDimensions = 3;
    TensorData.dimensions = { tensor.n_rows, tensor.n_cols,
        tensor.n_slices};
    TensorData.data.resize(tensor.n_elem);
    std::memcpy(TensorData.data.data(), tensor.memptr(),
        tensor.n_elem * sizeof(eT));
  }
protected:
  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    Base::Serialize(buffer, serialize);

    if (serialize)
    {
      // Verify data is correctly formatted
      if (TensorData.numDimensions != TensorData.dimensions.size())
      {
        Log::FatalError("Incorrect number of dimensions in tensor");
      }
      size_t numElem = 1;
      for (const uint64_t& len : TensorData.dimensions)
        numElem *= len;
      if (numElem != TensorData.data.size())
      {
        Log::FatalError("Incorrect number of elements in tensor");
      }
    }
    else
    {
      // Resize dimensions
      TensorData.dimensions.resize(TensorData.numDimensions);
    }

    mons::Serialize(buffer, TensorData.numDimensions, serialize);
    mons::Serialize(buffer, TensorData.dimensions, serialize);
    // Resize raw data buffer if deserializing
    size_t numElem = 1;
    if (!serialize)
    {
      for (const uint64_t& len : TensorData.dimensions)
        numElem *= len;
      TensorData.data.resize(numElem);
    }
    mons::Serialize(buffer, TensorData.data, serialize);

    buffer.Expect(sizeof(TensorData.numDimensions) +
        TensorData.numDimensions * sizeof(uint64_t) +
        numElem * sizeof(eT), serialize);
  };
};

} // namespace Message
} // namespace mons

#endif
