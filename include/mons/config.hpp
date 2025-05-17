/*
 * To set your own config, define the types
 * before including this file.
 */
#ifndef MONS_CONFIG_HPP
#define MONS_CONFIG_HPP

#include <mlpack/core/util/arma_traits.hpp>

#ifndef MONS_MAT_TYPE
  #define MONS_MAT_TYPE arma::mat
#endif

// See Common/dist_function.hpp
#ifndef MONS_FUNCTION_TYPE
  #define MONS_FUNCTION_TYPE mlpack::FFN<>
#endif

// Type of data used for predictors
#ifndef MONS_PREDICTOR_TYPE
  #define MONS_PREDICTOR_TYPE MONS_MAT_TYPE
#endif

// Type of data used for responses
#ifndef MONS_RESPONSE_TYPE
  #define MONS_RESPONSE_TYPE MONS_MAT_TYPE
#endif

// Type of data used for weights
#ifndef MONS_WEIGHT_TYPE
  #define MONS_WEIGHT_TYPE MONS_SEQUENCE_LENGTH_TYPE
#endif

// No touchy beyond this point
#define MONS_ELEM_TYPE MONS_MAT_TYPE::elem_type
#define MONS_CUBE_TYPE GetCubeType<MONS_MAT_TYPE>::type
#define MONS_ROW_TYPE GetRowType<MONS_MAT_TYPE>::type
#define MONS_SEQUENCE_LENGTH_TYPE arma::urowvec // RNN sequence lengths only accept arma::urowvec

#endif