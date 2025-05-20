#ifndef MONS_COMMON_SERIALIZE_HPP
#define MONS_COMMON_SERIALIZE_HPP

#include <vector>
#include <cstring>

#include "log.hpp"

// Utility to help serialize primitive types.
// Unsafe, make sure that the buffer contains data that is of type T.

namespace mons {
namespace Private {

enum DeserializeError {
  ERROR_BUFFER_OVERRUN
};

// This is set if there is an error while deserializing
size_t deserializeError = 0;

template <typename T>
struct SerializeLength
{
  static constexpr size_t value = sizeof(T);
};

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
void _SerializePrivate(std::vector<char>& buffer,
                       const T& val,
                       size_t start = -1)
{
  // Calculate start
  if (start == (size_t)-1)
    start = buffer.size();
  
  // Resize if necessary
  size_t newLen = start + Private::SerializeLength<T>::value;
  if (newLen > buffer.size())
  {
    buffer.resize(newLen);
  }

  // Write data
  char* bufferPtr = buffer.data() + start;
  memcpy(bufferPtr, &val, SerializeLength<T>::value);
  Private::_ToBigEndian(bufferPtr, Private::SerializeLength<T>::value);
}

// Convert bytes to T
template <typename T>
void _DeserializePrivate(const std::vector<char>& buffer,
                         T& val,
                         size_t& begin)
{
  // Size check
  if (buffer.size() < begin + Private::SerializeLength<T>::value)
  {
    Log::Error("Trying to deserialize (" + std::to_string(begin) +
        " + " + std::to_string(Private::SerializeLength<T>::value) +
        ") but buffer is only (" + std::to_string(buffer.size()) +
        ") bytes long.");
    deserializeError = DeserializeError::ERROR_BUFFER_OVERRUN;
    return;
  }
  // Write data
  memcpy(&val, buffer.data() + begin, SerializeLength<T>::value);
  _BigToNativeEndian((char*)&val, SerializeLength<T>::value);
  begin += Private::SerializeLength<T>::value;
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

// Serialize T to an existing buffer, automatically resizing buffer if necessary.
template <typename T>
void Serialize(std::vector<char>& buffer,
               const T& val,
               size_t start = -1,
               const typename std::enable_if_t<
                   !Private::HasSerialSpecialization<T>::value
               >* = 0)
{
  Private::_SerializePrivate(buffer, val, start);
}

// Deserialize T from a buffer.
template <typename T>
void Deserialize(const std::vector<char>& buffer,
                 T& val,
                 size_t& begin,
                 const typename std::enable_if_t<
                   !Private::HasSerialSpecialization<T>::value
                 >* = 0)
{
  Private::_DeserializePrivate(buffer, val, begin);
}

// Serialize vector specialization
template <typename T>
void Serialize(std::vector<char>& buffer,
               const T& val,
               size_t start = -1,
               const typename std::enable_if_t<
                 Private::IsVector<T>::value
               >* = 0)
{
  // Calculate offset here instead of depending on `_SerializePrivate` to calculate it
  if (start == (size_t)-1)
    start = buffer.size();
  
  for (size_t i = 0; i < val.size(); i++)
  {
    Serialize(buffer, val[i], start);
    start += sizeof(typename T::value_type);
  }
}

// Deserialize vector specialization
// Assumes `val` is already set to the correct size
template <typename T>
void Deserialize(const std::vector<char>& buffer,
                 T& val,
                 size_t& begin,
                 const typename std::enable_if_t<
                   Private::IsVector<T>::value
                 >* = 0)
{
  for (size_t i = 0; i < val.size(); i++)
  {
    // begin is increased in the below call
    Deserialize(buffer, val[i], begin);
  }
}

} // namespace mons

#endif
