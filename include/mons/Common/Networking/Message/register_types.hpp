/*
 * Macro for registering message types
 */
#ifndef MONS_COMMON_MESSAGE_REGISTER_TYPES_HPP
#define MONS_COMMON_MESSAGE_REGISTER_TYPES_HPP

// Only register types that are not inherited by others
#define MONS_REGISTER_MESSAGE_TYPES \
  REGISTER(HeartbeatMessage) \
  REGISTER(UpdatePredictorsMessage) \
  REGISTER(UpdateResponsesMessage) \
  REGISTER(UpdateWeightsMessage) \
  REGISTER(ShuffleMessage)

#endif
