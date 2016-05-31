#pragma once

#include <string>
#include <stdexcept>

struct Exception : public std::runtime_error
{
  explicit Exception(const std::string& error_message = "")
    : std::runtime_error(error_message.c_str()) {}
};

#define DefineChildException(ExName, ParentEx) struct ExName : public ParentEx { \
  explicit ExName(const std::string& error_message = "") \
    : ParentEx("["+ std::string(__FUNCTION__) +"] => "+ error_message) {} \
}
#define DefineBaseException(ExName) DefineChildException(ExName, Exception)
