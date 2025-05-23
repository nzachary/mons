#ifndef MONS_COMMON_SERIALIZE_HPP
#define MONS_COMMON_SERIALIZE_HPP

#include <vector>
#include <cstring>

#include "log.hpp"

// Utility to help serialize primitive types.
// Unsafe, make sure that the buffer contains data that is of type T.

namespace mons {

// Struct to hold a serialized message
struct MessageBuffer {
  // Raw character buffer
  std::vector<char> data;
  // Current head of deserializing
  size_t deserializePtr = 0;
  // Number of bytes read or written since last call to `Expect`
  size_t processed = 0;

  MessageBuffer(size_t size) : data(size) {}
  // Expect a number of bytes written or read and optionally throw
  void Expect(size_t n, bool thr) {
    if (n != processed)
    {
      Log::Error("Unexpected number of bytes read or written");
      if (thr)
        throw std::runtime_error("Unexpected number of bytes");
    }
    processed = 0;
  }
};

namespace Private {

enum DeserializeError {
  ERROR_BUFFER_OVERRUN
};

// This is set if there is an error while deserializing
size_t deserializeError = 0;

// Reverses `arr` if the system does not use big endian.
void _ToBigEndian(char* arr, size_t n)
{
  #if (__ORDER_BIG_ENDIAN__ != __BYTE_ORDER__)
    for (size_t i = 0; i < n; i++)
      std::swap(arr[i], arr[n - i - 1]);
  #endif
}

// Assumes `arr` is a byte array of something in big endian.
void _BigToNativeEndian(char* arr, size_t n)
{
  // Since _ToBigEndian is just a reversing operation, reverse it again to get the original.
    _ToBigEndian(arr, n);
}

// Implementation
// Convert T to bytes, automatically resizing buffer if necessary.
template <typename T>
void _SerializePrivate(MessageBuffer& buffer,
                       const T& val,
                       size_t start = -1)
{
  // Calculate start
  if (start == (size_t)-1)
    start = buffer.data.size();
  
  // Resize if necessary
  size_t newLen = start + sizeof(T);
  if (newLen > buffer.data.size())
  {
    buffer.data.resize(newLen);
  }

  // Write data
  char* bufferPtr = buffer.data.data() + start;
  memcpy(bufferPtr, &val, sizeof(T));
  Private::_ToBigEndian(bufferPtr, sizeof(T));
  buffer.processed += sizeof(T);
}

// Convert bytes to T
template <typename T>
void _DeserializePrivate(MessageBuffer& buffer,
                         T& val,
                         size_t start = -1)
{
  // Calculate start
  if (start == (size_t)-1)
    start = buffer.deserializePtr;
  
  // Size check
  size_t end = buffer.deserializePtr + sizeof(T);
  if (buffer.data.size() < end)
  {
    deserializeError = DeserializeError::ERROR_BUFFER_OVERRUN;
    return;
  }
  // Write data
  char* bufferPtr = buffer.data.data() + start;
  void* ptr = (void*)const_cast<T*>(&val);
  memcpy(ptr, bufferPtr, sizeof(T));
  _BigToNativeEndian((char*)ptr, sizeof(T));
  buffer.deserializePtr += sizeof(T);
  buffer.processed += sizeof(T);
}

// SFINAE structs
template <typename T>
struct IsVector
{
  static const bool value = false;
};

template <typename T>
struct IsVector<std::vector<T>>
{
  static const bool value = true;
};

template <typename T>
struct HasSerialSpecialization
{
  static const bool value = IsVector<T>::value;
};

} // namespace Private

// Returns true if there was an error while resetting the error flag
bool CheckDeserializeError(size_t& error)
{
  error = Private::deserializeError;
  Private::deserializeError = 0;
  return (error != 0);
}

bool CheckDeserializeError()
{
  size_t t;
  return CheckDeserializeError(t);
}

// Serialize or deserialize T from a buffer
// If serializing, the buffer will be automatically extended
// Direction is true if serializing and false if deserializing
template <typename T>
void Serialize(MessageBuffer& buffer,
               const T& val,
               bool direction,
               size_t start = -1,
               const typename std::enable_if_t<
                   !Private::HasSerialSpecialization<T>::value
               >* = 0)
{
  if (direction)
  {
    Private::_SerializePrivate(buffer, val, start);
  }
  else
  {
    Private::_DeserializePrivate(buffer, val, start);
  }
}

// Serialize vector specialization
template <typename T>
void Serialize(MessageBuffer& buffer,
               const T& val,
               bool direction,
               size_t start = -1,
               const typename std::enable_if_t<
                 Private::IsVector<T>::value
               >* = 0)
{
  // Calculate offset here instead of depending on `_SerializePrivate` to calculate it
    if (start == (size_t)-1)
      start = (direction) ? buffer.data.size() : buffer.deserializePtr;
  
  for (size_t i = 0; i < val.size(); i++)
  {
    Serialize(buffer, val[i], direction, start);
    start += sizeof(typename T::value_type);
  }
}

} // namespace mons

#endif
