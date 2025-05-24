/*
 * Message containing a dense tensor
 */
#ifndef MONS_COMMON_MESSAGE_TENSOR_HPP
#define MONS_COMMON_MESSAGE_TENSOR_HPP

#include <armadillo>
#include <mlpack/core/util/arma_traits.hpp>

#include "../../config.hpp"
#include "../enum_elem_map.hpp"
#include "base.hpp"

namespace mons {
namespace Message {

class Tensor : public Base
{
public:
  struct TensorDataStruct
  {
    // Type of element data
    uint16_t dataType;
    // Number of dimensions
    uint16_t numDimensions;
    // Lengths of each dimension
    std::vector<uint32_t> dimensions;
    // Element data
    // This is parsed into the target element type elsewhere
    std::vector<char> data;
  } TensorData;

  // Get data as a vector of type T instead of char
  template <typename T>
  static std::vector<T> DataAsType(const TensorDataStruct& TensorData)
  {
    if (TypeInfo<T>::num != TensorData.dataType)
    {
      Log::Error("Tensor data has type " + GetTypeName(TensorData.dataType) +
          " but trying to parse as type " + GetTypeName<T>());
      return {};
    }
    size_t nElem = TensorData.data.size() / GetTypeSize(TensorData.dataType);
    std::vector<T> out(nElem);
    // Deserialize data again but this time with a known type
    MessageBuffer tempBuffer(0);
    tempBuffer.data = TensorData.data;
    mons::Serialize(tempBuffer, out, false);
    return out;
  }

  // TODO: bandicoot types
  // Gets the contained tensor as type T
  template <typename T>
  static void GetTensor(T& tensor, const TensorDataStruct& TensorData,
                 const typename std::enable_if_t<
                   IsVector<T>::value
                 >* = 0)
  {
    using eT = typename T::elem_type;
    if (TensorData.numDimensions != 1)
    {
      Log::Error("Trying to get tensor as row but dimensionality is " +
          std::to_string(TensorData.numDimensions));
      new (&tensor) T();
      return;
    }
    std::vector<eT> typedVector = Tensor::DataAsType<eT>(TensorData);
    
    // Placement new to avoid copy operation
    new (&tensor) T(typedVector.data(),
        TensorData.dimensions[0]);
  }

  template <typename T>
  static void GetTensor(T& tensor, const TensorDataStruct& TensorData,
                 const typename std::enable_if_t<
                   IsMatrix<T>::value
                 >* = 0)
  {
    using eT = typename T::elem_type;
    if (TensorData.numDimensions != 2)
    {
      Log::Error("Trying to get tensor as matrix but dimensionality is " +
          std::to_string(TensorData.numDimensions));
      new (&tensor) T();
      return;
    }
    std::vector<eT> data = Tensor::DataAsType<eT>(TensorData);
    
    // Placement new to avoid copy operation
    new (&tensor) T(data.data(),
        TensorData.dimensions[0], TensorData.dimensions[1]);
  }

  template <typename T>
  static void GetTensor(T& tensor, const TensorDataStruct& TensorData,
                 const typename std::enable_if_t<
                   IsCube<T>::value
                 >* = 0)
  {
    using eT = typename T::elem_type;
    if (TensorData.numDimensions != 3)
    {
      Log::Error("Trying to get tensor as cube but dimensionality is " +
          std::to_string(TensorData.numDimensions));
      new (&tensor) T();
      return;
    }
    std::vector<eT> data = Tensor::DataAsType<eT>(TensorData);
    
    // Placement new to avoid copy operation
    new (&tensor) T(data.data(), TensorData.dimensions[0],
        TensorData.dimensions[1], TensorData.dimensions[2]);
  }

  // Sets the tensor to be sent
  template <typename T>
  static void SetTensor(const T& tensor, TensorDataStruct& TensorData,
                 const typename std::enable_if_t<
                   IsVector<T>::value
                 >* = 0)
  {
    using eT = typename T::elem_type;
    TensorData.dataType = TypeInfo<eT>::num;
    TensorData.numDimensions = 1;
    TensorData.dimensions = { (uint32_t)tensor.n_elem };
    // Write element data into a temporary typed vector
    std::vector<eT> typedVector(tensor.n_elem);
    std::memcpy(typedVector.data(), tensor.memptr(),
        tensor.n_elem * sizeof(eT));
    // Serialize typed buffer
    MessageBuffer tempBuffer(0);
    mons::Serialize(tempBuffer, typedVector, true);
    // Move serialized data into `TensorData`
    TensorData.data = std::move(tempBuffer.data);
  }

  template <typename T>
  static void SetTensor(const T& tensor, TensorDataStruct& TensorData,
                 const typename std::enable_if_t<
                   IsMatrix<T>::value
                 >* = 0)
  {
    using eT = typename T::elem_type;
    TensorData.dataType = TypeInfo<eT>::num;
    TensorData.numDimensions = 2;
    TensorData.dimensions = { (uint32_t)tensor.n_rows,
        (uint32_t)tensor.n_cols };
    // Write element data into a temporary typed vector
    std::vector<eT> typedVector(tensor.n_elem);
    std::memcpy(typedVector.data(), tensor.memptr(),
        tensor.n_elem * sizeof(eT));
    // Serialize typed buffer
    MessageBuffer tempBuffer(0);
    mons::Serialize(tempBuffer, typedVector, true);
    // Move serialized data into `TensorData`
    TensorData.data = std::move(tempBuffer.data);
  }

  template <typename T>
  static void SetTensor(const T& tensor, TensorDataStruct& TensorData,
                 const typename std::enable_if_t<
                   IsCube<T>::value
                 >* = 0)
  {
    using eT = typename T::elem_type;
    TensorData.dataType = TypeInfo<eT>::num;
    TensorData.numDimensions = 3;
    TensorData.dimensions = { (uint32_t)tensor.n_rows, (uint32_t)tensor.n_cols,
        (uint32_t)tensor.n_slices};
    // Write element data into a temporary typed vector
    std::vector<eT> typedVector(tensor.n_elem);
    std::memcpy(typedVector.data(), tensor.memptr(),
        tensor.n_elem * sizeof(eT));
    // Serialize typed buffer
    MessageBuffer tempBuffer(0);
    mons::Serialize(tempBuffer, typedVector, true);
    // Move serialized data into `TensorData`
    TensorData.data = std::move(tempBuffer.data);
  }

protected:
  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    Base::Serialize(buffer, serialize);

    // Get size of each element from dataType
    mons::Serialize(buffer, TensorData.dataType, serialize);
    size_t elemSize = GetTypeSize(TensorData.dataType);

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
      if (numElem * elemSize != TensorData.data.size())
      {
        Log::FatalError("Incorrect number of elements in tensor");
      }
    }

    mons::Serialize(buffer, TensorData.numDimensions, serialize);
    if (!serialize)
      TensorData.dimensions.resize(TensorData.numDimensions);
    mons::Serialize(buffer, TensorData.dimensions, serialize);
    size_t numElem = 1;
    for (const uint64_t& len : TensorData.dimensions)
      numElem *= len;
    if (!serialize)
      TensorData.data.resize(numElem * elemSize);
    mons::Serialize(buffer, TensorData.data, serialize);

    buffer.Expect(sizeof(TensorData.dataType) +
        sizeof(TensorData.numDimensions) +
        TensorData.numDimensions * sizeof(uint32_t) +
        numElem * elemSize, serialize);
  };
};

} // namespace Message
} // namespace mons

#endif
