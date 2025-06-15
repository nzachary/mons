/*
 * Lockable wrapper class
 */
#ifndef MONS_COMMON_SAFE_REF_HPP
#define MONS_COMMON_SAFE_REF_HPP

#include <memory>
#include <mutex>
#include <functional>

namespace mons {

template <typename T>
class SafeRef {
public:
  std::unique_lock<std::mutex> Lock();

  SafeRef(T& val);
  SafeRef(SafeRef<T>& other);

  T& Value();
  SafeRef<T>& operator=(T& val);
  SafeRef<T>& operator=(SafeRef<T>& other);
private:
  std::reference_wrapper<T> ref;

  std::shared_ptr<std::mutex> mutex;
};

} // namespace mons

#include "safe_ref_impl.hpp"

#endif
