/*
 * Checks if a function uses a train form
 */
#ifndef MONS_COMMON_HAS_TRAIN_FORM_HPP
#define MONS_COMMON_HAS_TRAIN_FORM_HPP

namespace mons {

template <typename FuncType>
struct UseTrain2
{
  const static bool value = false;
};
template <typename FuncType>
struct UseTrain3
{
  const static bool value = false;
};

template <typename... FFNArgs>
struct UseTrain2<mlpack::FFN<FFNArgs...>>
{
  const static bool value = true;
};

template <typename... RNNArgs>
struct UseTrain3<mlpack::RNN<RNNArgs...>>
{
  const static bool value = true;
};

} // namespace mons

#endif
