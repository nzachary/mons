/*
 * Default configuration
 * To set your own config, you can either 1) modify this file or 2) define
 * MONS_CUSTOM_CONFIG all of the following:
 * MONS_MAT_TYPE
 * MONS_FUNCTION_TYPE
 * MONS_PREDICTOR_TYPE
 * MONS_RESPONSE_TYPE
 * MONS_WEIGHT_TYPE
 * MONS_PREDICTOR_NAME
 * MONS_RESPONSE_NAME
 * MONS_WEIGHT_NAME
 * 
 * Do not set the _NAME defines if they aren't used. eg. FFN doesn't have a
 * weights parameter so MONS_WEIGHT_NAME is not defined
 */
#ifndef MONS_CONFIG_HPP
#define MONS_CONFIG_HPP

#ifndef MONS_CUSTOM_CONFIG

#define MONS_MAT_TYPE arma::mat

// See Common/dist_function.hpp
#define MONS_FUNCTION_TYPE mlpack::FFN<mlpack::MeanSquaredError>

// Type of data used for predictors
#define MONS_PREDICTOR_TYPE MONS_MAT_TYPE
#define MONS_RESPONSE_TYPE MONS_MAT_TYPE
#define MONS_WEIGHT_TYPE MONS_SEQUENCE_LENGTH_TYPE

// Names of member variables
#define MONS_PREDICTOR_NAME predictors
#define MONS_RESPONSE_NAME responses
//#define MONS_WEIGHT_NAME

#endif

// No touchy beyond this point
#include <mlpack/core/util/arma_traits.hpp>

#define MONS_ELEM_TYPE MONS_MAT_TYPE::elem_type
#define MONS_CUBE_TYPE GetCubeType<MONS_MAT_TYPE>::type
#define MONS_ROW_TYPE GetRowType<MONS_MAT_TYPE>::type
#define MONS_SEQUENCE_LENGTH_TYPE arma::urowvec // RNN sequence lengths only accept arma::urowvec

#endif