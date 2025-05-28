/*
 * Class to help with logging
 */
#ifndef MONS_COMMON_LOG_HPP
#define MONS_COMMON_LOG_HPP

#include <string>
#include <iostream>

#include "../config.hpp"

namespace mons {

class Log
{
public:
  // Log a debug message
  static void Debug(const std::string& message)
  {
  #ifndef NDEBUG
    WriteMessage("[Debug]: " + message);
  #endif
  }
  // Log a status message
  static void Status(const std::string& message)
  {
    WriteMessage("[Status]: " + message);
  }
  // Log a warning message
  static void Warning(const std::string& message)
  {
    WriteMessage("[Warning]: " + message);
  }
  // Log an error message
  static void Error(const std::string& message)
  {
    WriteMessage("[Error]: " + message);
  }
  // Log a fatal error message
  static void FatalError(const std::string& message)
  {
    WriteMessage("[Fatal error]: " + message);
    throw std::runtime_error(message);
  }
private:
  // Write a message to stream
  static void WriteMessage(const std::string& message)
  {
    MONS_LOG_STREAM << message << std::endl;
  }
};

} // namespace mons

#endif
