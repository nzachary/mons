/*
 * Default configuration
 * To set your own config, you can either 1) modify this file or
 * 2) define the following:
 * MONS_MAT_TYPE
 * MONS_FUNCTION_TYPE
 * MONS_PREDICTOR_TYPE
 * MONS_RESPONSE_TYPE
 * MONS_WEIGHT_TYPE
 * MONS_LOG_STREAM
 */
#ifndef MONS_CONFIG_HPP
#define MONS_CONFIG_HPP

#ifndef MONS_MAT_TYPE
  #define MONS_MAT_TYPE arma::mat
#endif

// See Common/dist_function.hpp
#ifndef MONS_FUNCTION_TYPE
  #define MONS_FUNCTION_TYPE mlpack::FFN<mlpack::MeanSquaredError>
#endif

// Type of data used for predictors
#ifndef MONS_PREDICTOR_TYPE
  #define MONS_PREDICTOR_TYPE MONS_MAT_TYPE
#endif
#ifndef MONS_RESPONSE_TYPE
  #define MONS_RESPONSE_TYPE MONS_MAT_TYPE
#endif
#ifndef MONS_WEIGHT_TYPE
  #define MONS_WEIGHT_TYPE MONS_SEQUENCE_LENGTH_TYPE
#endif

#ifndef MONS_LOG_STREAM
  #define MONS_LOG_STREAM std::cout
#endif

// No touchy beyond this point
#include <mlpack/core/util/arma_traits.hpp>

#define MONS_ELEM_TYPE typename MONS_MAT_TYPE::elem_type
#define MONS_CUBE_TYPE typename GetCubeType<MONS_MAT_TYPE>::type
#define MONS_ROW_TYPE typename GetRowType<MONS_MAT_TYPE>::type
#define MONS_SEQUENCE_LENGTH_TYPE arma::urowvec // RNN sequence lengths only accept arma::urowvec

#endif