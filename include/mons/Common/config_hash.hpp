/*
 * Calculates a hash of the config so that incompatible configurations
 * aren't used with each other. It doesn't have to be secure, just very
 * unlikely that 2 different but functional configurations have the same hash.
 */
#ifndef MONS_COMMON_CONFIG_HASH_HPP
#define MONS_COMMON_CONFIG_HASH_HPP

#include <string>
#include <cstdint>

#include "../config.hpp"
#include "../version.hpp"

#define QUOTE(v) #v
#define STRINGIFY(v) QUOTE(v)
#define STRINGIFY2(v) STRINGIFY(v)

namespace mons {
namespace Private {

uint32_t GetConfigHash()
{
  uint32_t res = 0;
  std::string confStr = 
      STRINGIFY(MONS_MAT_TYPE)
      STRINGIFY(MONS_FUNCTION_TYPE)
      STRINGIFY(MONS_PREDICTOR_TYPE)
      STRINGIFY(MONS_RESPONSE_TYPE)
      STRINGIFY(MONS_WEIGHT_TYPE)
      STRINGIFY(MONS_PREDICTOR_NAME)
      STRINGIFY(MONS_RESPONSE_NAME)
      STRINGIFY(MONS_WEIGHT_NAME)
      STRINGIFY(MONS_VERSION_NUMBER);

  size_t stepWidth = sizeof(res) / sizeof(confStr[0]);
  // Resize to have a multiple of `stepWidth` chars in `confStr`
  {
    size_t rem = confStr.size() % stepWidth;
    if (rem > 0)
      confStr.resize((confStr.size() / stepWidth + 1) * stepWidth, 'a');
  }

  // Calculate result from string
  size_t numSteps = confStr.size() / stepWidth;
  for (size_t i = 0; i < numSteps; i++)
  {
    for (size_t j = 0; j < stepWidth; j++)
    {
      char byte = confStr[i * numSteps + j];
      res ^ (byte << j * 8);
    }
  }

  return res;
}

} // namespace Private

const uint32_t CONFIG_HASH = Private::GetConfigHash();

} // namespace mons

#undef STRINGIFY
#undef QUOTE

#endif
