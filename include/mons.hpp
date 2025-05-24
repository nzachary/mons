#ifndef MONS_HPP
#define MONS_HPP

#define MLPACK_ENABLE_ANN_SERIALIZATION

#include <mlpack.hpp>

#include "mons/version.hpp"
#include "mons/config.hpp"

#include "mons/common.hpp"

// Define MONS_DONT_INCLUDE_CLIENT if you don't need client parts
#ifndef MONS_DONT_INCLUDE_CLIENT
  #include "mons/client.hpp"
#endif

// Define MONS_DONT_INCLUDE_SERVER if you don't need server parts
#ifndef MONS_DONT_INCLUDE_SERVER
  #include "mons/server.hpp"
#endif

#endif
