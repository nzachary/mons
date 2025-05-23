/*
 * Helper class to assist with illegally accessing internal mlpack members
 */
#ifndef MONS_CLIENT_FUNCTION_ACCESSOR_HPP
#define MONS_CLIENT_FUNCTION_ACCESSOR_HPP

#include <mlpack.hpp>

#include "../config.hpp"
#include "../Common/log.hpp"

#define Q(v) #v
#define QQ(v) Q(v)

namespace mons {

namespace Private {

// ========== https://github.com/hliberacki/cpp-member-accessor ==========
#pragma region Member Accessor
/*
MIT License

Copyright (c) 2018 Hubert Liberacki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//************************************************************************************
// Copyright Hubert Liberacki (hliberacki@gmail.com)
// Copyright Krzysztof Ostrowski
//
// Project home: https://github.com/hliberacki/cpp-member-accessor
//
// MIT LICENSE : https://github.com/hliberacki/cpp-member-accessor/blob/master/LICENSE
//************************************************************************************

#ifndef ACCESSOR_INCLUDE_ACCESSOR_HPP
#define ACCESSOR_INCLUDE_ACCESSOR_HPP

#include <functional>

namespace accessor
{
    template<typename C, typename T>
    struct MemberWrapper
    {
      using type = T (C::*);
    };

    template<class C, class R, typename... Args>
    struct FunctionWrapper
    {
      using type = R (C::*)(Args...);
    };

    template<class C, class R, typename... Args>
    struct ConstFunctionWrapper
    {
      using type = R (C::*)(Args...) const;
    };

    template<class Tag, class T>
    struct Proxy
    {
      static typename T::type value;
    };

    template <class Tag, class T>
    typename T::type Proxy<Tag, T>::value;

    template<class T, typename T::type AccessPointer>
    class MakeProxy
    {
      struct Setter { Setter() { Proxy<T, T>::value = AccessPointer; } };
      static Setter instance;
    };

    template<class T, typename T::type AccessPointer>
    typename MakeProxy<T, AccessPointer>::Setter MakeProxy<T, AccessPointer>::instance;

    template<typename Sig, class Instance, typename... Args>
    auto callFunction(Instance & instance, Args ...args)
    {
      return (instance.*(Proxy<Sig, Sig>::value))(args...);
    }

    template<typename Sig, class Instance>
    auto accessMember(Instance & instance)
    {
        return std::ref(instance.*(Proxy<Sig, Sig>::value));
    }
}

#define FUNCTION_HELPER(...)                                           \
  accessor::FunctionWrapper<__VA_ARGS__>

#define CONST_FUNCTION_HELPER(...)                                     \
  accessor::ConstFunctionWrapper<__VA_ARGS__>

#define FUNCTION_ACCESSOR(accessor_name, base, method, ...)            \
  struct accessor_name : FUNCTION_HELPER(base, __VA_ARGS__) {};            \
  template class accessor::MakeProxy<accessor_name, &base::method>;

#define CONST_FUNCTION_ACCESSOR(accessor_name, base, method, ...)      \
  struct accessor_name : CONST_FUNCTION_HELPER(base, __VA_ARGS__) {};      \
  template class accessor::MakeProxy<accessor_name, &base::method>;

#define MEMBER_ACCESSOR(accessor_name, base, member, ret_type)         \
  struct accessor_name : accessor::MemberWrapper<base, ret_type> {};       \
  template class accessor::MakeProxy<accessor_name, &base::member>;

#endif // ACCESSOR_INCLUDE_ACCESSOR_HPP

// ========== https://github.com/hliberacki/cpp-member-accessor ==========
#pragma endregion

// Access training parameters
#ifdef MONS_PREDICTOR_NAME
MEMBER_ACCESSOR(PredictorAccessor, MONS_FUNCTION_TYPE,
    MONS_PREDICTOR_NAME, MONS_PREDICTOR_TYPE);
#endif
#ifdef MONS_RESPONSE_NAME
MEMBER_ACCESSOR(ResponseAccessor, MONS_FUNCTION_TYPE,
    MONS_RESPONSE_NAME, MONS_RESPONSE_TYPE);
#endif
#ifdef MONS_WEIGHT_NAME
MEMBER_ACCESSOR(WeightAccessor, MONS_FUNCTION_TYPE,
    MONS_WEIGHT_NAME, MONS_WEIGHT_TYPE);
#endif

// Initalizer helpers
// CheckNetwork initalizes FFN
FUNCTION_ACCESSOR(FFNCheckNetwork, MONS_FUNCTION_TYPE, CheckNetwork, void, const std::string&, size_t, bool, bool)
// RNN calls its FFN's CheckNetwork to initalize
MEMBER_ACCESSOR(RNNNetworkGetter, mlpack::RNN<>, network, mlpack::FFN<>)

// Clean up member accessor macros, we don't need them elsewhere
#undef FUNCTION_HELPER
#undef CONST_FUNCTION_HELPER
#undef FUNCTION_ACCESSOR
#undef CONST_FUNCTION_ACCESSOR
#undef MEMBER_ACCESSOR

} // namespace Private

class FunctionAccessor
{
public:
  friend class DistFunctionClient;

  // Get the underlying function
  MONS_FUNCTION_TYPE& Get() { return function; };

  // Initalize the function outside of training
  // Generic initalizer
  template <typename T>
  void Initalize(T&)
  {
    Log::Error("Function is missing specalized initalizer");
  }
  // FFN initalizer
  template <typename... FFNArgs>
  void Initalize(mlpack::FFN<FFNArgs...>& ffn)
  {
    auto& predictors = Private::accessor::accessMember<
        Private::PredictorAccessor>(function).get();
    ffn.InputDimensions() = inputDims;
    size_t inputSize = 1;
    for (size_t d : inputDims)
      inputSize *= d;
    Private::accessor::callFunction<Private::FFNCheckNetwork>(ffn,
        "mons::FunctionAccessor::Initalize()", inputSize, false, false);
  }
  // RNN initalizer
  template <typename... RNNArgs>
  void Initalize(mlpack::RNN<RNNArgs...>& rnn)
  {
    // Call the FFN initalizer using the RNN's FFN
    Initalize(Private::accessor::accessMember<
        Private::RNNNetworkGetter>(rnn).get());
  }

  void SetPredictors(MONS_PREDICTOR_TYPE& predictors) {
#ifdef MONS_PREDICTOR_NAME
    Private::accessor::accessMember<Private::PredictorAccessor>(function)
        .get() = predictors;
#else
    Log::Error(QQ(MONS_FUNCTION_TYPE) " has no setter for predictors");
#endif
  };

  void SetResponses(MONS_RESPONSE_TYPE& responses) {
#ifdef MONS_RESPONSE_NAME
    Private::accessor::accessMember<Private::ResponseAccessor>(function)
        .get() = responses;
#else
    Log::Error(QQ(MONS_FUNCTION_TYPE) " has no setter for responses");
#endif
  };

  void SetWeights(MONS_WEIGHT_TYPE& weights) {
#ifdef MONS_WEIGHT_NAME
    Private::accessor::accessMember<Private::WeightAccessor>(function)
        .get() = weights;
#else
    Log::Error(QQ(MONS_FUNCTION_TYPE) " has no setter for weights");
#endif
  };

  // Sets input dimensions, for internal use
  template <typename T>
  void _SetInputDims(const std::vector<T>& dims)
  {
    inputDims.resize(dims.size());
    for (size_t i = 0; i < dims.size(); i++)
      inputDims[i] = dims[i];
  }
private:
  MONS_FUNCTION_TYPE function;

  std::vector<size_t> inputDims;
};

} // namespace mons

#undef QQ
#undef Q

#endif
