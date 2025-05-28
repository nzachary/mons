
#ifndef MONS_COMMON_ENUM_ELEM_MAP
#define MONS_COMMON_ENUM_ELEM_MAP

#include <stdlib.h>
#include <string>
#include <stdexcept>
#include <armadillo>

namespace mons {

#define REGISTER_TYPES \
  REGISTER(FLOAT, float) \
  REGISTER(DOUBLE, double) \
  REGISTER(SIZE_T, size_t) \
  REGISTER(UWORD, arma::uword)

#define REGISTER(Name,Type) Name,
enum ElemType {
  REGISTER_TYPES
};
#undef REGISTER

// Type to enum conversion
template <typename T>
struct TypeInfo
{
  static const int num = -1;
  inline static const std::string name = "unknown";
};

#define REGISTER(Name,Type) \
template<> \
struct TypeInfo<Type> \
{ \
  static const int num = Name; \
  inline static const std::string name = "Type"; \
};
REGISTER_TYPES
#undef REGISTER

// Enum to type conversion
template <int val>
struct GetType;

#define REGISTER(Name,Type) \
template<> \
struct GetType<Name> \
{ \
  using type = Type; \
};
REGISTER_TYPES
#undef REGISTER

// Helper functions
std::string GetTypeName(int num)
{
  #define REGISTER(Name,Type) case ElemType::Name: return \
      TypeInfo<Type>::name;
  switch (num)
  {
    REGISTER_TYPES
    default:
      throw std::runtime_error("Type not defined");
      return "undefined";
  }
  #undef REGISTER
}

size_t GetTypeSize(int num)
{
  #define REGISTER(Name,Type) case ElemType::Name: return \
      sizeof(Type);
  switch (num)
  {
    REGISTER_TYPES
    default:
      throw std::runtime_error("Type not defined");
      return -1;
  }
  #undef REGISTER
}

template <typename T>
std::string GetTypeName()
{
  int num = TypeInfo<T>::num;
  return GetTypeName(num);
}

template <typename T>
size_t GetTypeSize()
{
  int num = TypeInfo<T>::num;
  return GetTypeSize(num);
}

#undef REGISTER_TYPES

} // namespace mons

#endif
