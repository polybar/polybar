#pragma once

#ifdef DEBUG
#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2
#endif

#include <boost/di.hpp>
#include <cerrno>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "config.hpp"

#define POLYBAR_NS    \
  namespace polybar { \
    inline namespace v2_0_0 {
#define POLYBAR_NS_END \
  }                    \
  }
#define POLYBAR_NS_PATH "polybar::v2_0_0"

#ifndef PIPE_READ
#define PIPE_READ 0
#endif
#ifndef PIPE_WRITE
#define PIPE_WRITE 1
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifdef DEBUG
#include "debug.hpp"
#endif

POLYBAR_NS

//==================================================
// Include common types (i.e, unclutter editor!)
//==================================================

namespace di = boost::di;
namespace placeholders = std::placeholders;

using std::string;
using std::stringstream;
using std::size_t;
using std::move;
using std::bind;
using std::forward;
using std::pair;
using std::function;
using std::shared_ptr;
using std::unique_ptr;
using std::make_unique;
using std::make_shared;
using std::make_pair;
using std::array;
using std::map;
using std::vector;
using std::to_string;
using std::strerror;
using std::exception;

//==================================================
// Errors and exceptions
//==================================================

class application_error : public std::runtime_error {
 public:
  int m_code;

  explicit application_error(string&& message, int code = 0)
      : std::runtime_error(forward<string>(message)), m_code(code) {}
};

class system_error : public application_error {
 public:
  explicit system_error() : application_error(strerror(errno), errno) {}
  explicit system_error(string&& message)
      : application_error(forward<string>(message) + " (reason: " + strerror(errno) + ")", errno) {}
};

#define DEFINE_CHILD_ERROR(error, parent) \
  class error : public parent {           \
    using parent::parent;                 \
  }
#define DEFINE_ERROR(error) DEFINE_CHILD_ERROR(error, application_error)

POLYBAR_NS_END
