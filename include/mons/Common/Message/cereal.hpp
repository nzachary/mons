/*
 * Message containing a cereal archive
 */
#ifndef MONS_COMMON_MESSAGE_CEREAL_HPP
#define MONS_COMMON_MESSAGE_CEREAL_HPP

#include <sstream>

#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/binary.hpp>

#include "base.hpp"

namespace mons {
namespace Message {

class Cereal : public Base
{
public:
  struct CerealDataStruct
  {
    // Length of binarized data
    uint64_t length;
    // Binarized data
    std::vector<char> data;
  } CerealData;

  // Write an object that can be serialized by cereal into the message
  template <typename T>
  static bool Cerealize(const T& object, CerealDataStruct& CerealData)
  {
    std::stringstream ss;
    try
    {
      // Write data into string stream
      cereal::BinaryOutputArchive archive(ss);
      archive(object);
    }
    catch(std::exception& e)
    {
      Log::Error(e.what());
      return false;
    }
    
    // Put contents of string stream into data
    std::string str = ss.str();
    CerealData.length = str.size();
    CerealData.data.resize(CerealData.length);
    std::memcpy(CerealData.data.data(), str.c_str(), CerealData.length);
    return true;
  }

  // Read an object that can be serialized by cereal from the message
  template <typename T>
  static bool Decerealize(T& object, const CerealDataStruct& CerealData)
  {
    // Read data into string stream
    std::string str(CerealData.data.data(), CerealData.length);
    std::stringstream ss(str);
    try
    {
      // Load object from stream
      cereal::BinaryInputArchive archive(ss);
      archive(cereal::make_nvp("m", object));
    }
    catch(std::exception& e)
    {
      Log::Error(e.what());
      return false;
    }
    return true;
  }
protected:
  virtual void Serialize(MessageBuffer& buffer, bool serialize)
  {
    Base::Serialize(buffer, serialize);

    mons::Serialize(buffer, CerealData.length, serialize);
    if (!serialize)
      CerealData.data.resize(CerealData.length);
    mons::Serialize(buffer, CerealData.data, serialize);

    buffer.Expect(sizeof(CerealDataStruct::length) +
        CerealData.length * sizeof(char), serialize);
  }
};

} // namespace Message
} // namespace mons

#endif
