#ifndef MONS_COMMON_SAFE_REF_IMPL_HPP
#define MONS_COMMON_SAFE_REF_IMPL_HPP

#include "safe_ref.hpp"

namespace mons {

template <typename T>
std::unique_lock<std::mutex> SafeRef<T>::Lock()
{
  return std::unique_lock(*mutex);
}

template <typename T>
SafeRef<T>::SafeRef(T& val) :
    ref(val)
{
  mutex = std::make_shared<std::mutex>();
}

template <typename T>
SafeRef<T>::SafeRef(SafeRef<T>& other) :
    ref(other.ref)
{
  // Share a mutex with the other mutex
  mutex = other.mutex;
}

template <typename T>
T& SafeRef<T>::Value()
{
  return ref;
}

template <typename T>
SafeRef<T>& SafeRef<T>::operator=(T& val)
{
  ref = val;
  return *this;
}

template <typename T>
SafeRef<T>& SafeRef<T>::operator=(SafeRef<T>& other)
{
  ref = other.ref;
  mutex = other.mutex;
  return *this;
}

} // namespace mons

#endif