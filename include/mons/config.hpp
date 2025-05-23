/*
 * To set your own config, define all of the types and 
 * before including this file.
 */
#ifndef MONS_CONFIG_HPP
#define MONS_CONFIG_HPP

#include <mlpack/core/util/arma_traits.hpp>

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
#define MONS_ELEM_TYPE MONS_MAT_TYPE::elem_type
#define MONS_CUBE_TYPE GetCubeType<MONS_MAT_TYPE>::type
#define MONS_ROW_TYPE GetRowType<MONS_MAT_TYPE>::type
#define MONS_SEQUENCE_LENGTH_TYPE arma::urowvec // RNN sequence lengths only accept arma::urowvec

#endif