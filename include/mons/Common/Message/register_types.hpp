/*
 * Macro for registering message types
 */
#ifndef MONS_COMMON_MESSAGE_REGISTER_TYPES_HPP
#define MONS_COMMON_MESSAGE_REGISTER_TYPES_HPP

// Only register types that are not inherited by others
// Otherwise, there may be ambiguity when casting
#define MONS_REGISTER_MESSAGE_TYPES \
  REGISTER(UpdatePredictors) \
  REGISTER(UpdateResponses) \
  REGISTER(UpdateWeights) \
  REGISTER(Shuffle) \
  REGISTER(EvaluateWithGradient) \
  REGISTER(Gradient) \
  REGISTER(OperationStatus) \
  REGISTER(UpdateFunction) \
  REGISTER(UpdateParameters) \
  REGISTER(ConnectInfo)

#endif
