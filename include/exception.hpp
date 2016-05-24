#ifndef _EXCEPTION_HPP_
#define _EXCEPTION_HPP_

#include <string>
#include <stdexcept>

struct Exception : public std::runtime_error
{
  Exception(std::string const &error_message = "")
    : std::runtime_error(error_message.c_str()) {}
};

#define DefineChildException(ExName, ParentEx) struct ExName : public ParentEx { \
  ExName(std::string error_message = "") \
    : ParentEx("["+ std::string(__FUNCTION__) +"] => "+ error_message) {} \
}
#define DefineBaseException(ExName) DefineChildException(ExName, Exception)

#endif

