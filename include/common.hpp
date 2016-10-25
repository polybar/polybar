#pragma once

#ifdef DEBUG
#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2
#endif

#include <atomic>
#include <boost/di.hpp>
#include <boost/optional.hpp>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "config.hpp"

#define LEMONBUDDY_NS    \
  namespace lemonbuddy { \
    inline namespace v2_0_0 {
#define LEMONBUDDY_NS_END \
  }                       \
  }
#define LEMONBUDDY_NS_PATH "lemonbuddy::v2_0_0"

#define PIPE_READ 0
#define PIPE_WRITE 1

#ifdef DEBUG
#include "debug.hpp"
#endif

LEMONBUDDY_NS

//==================================================
// Include common types (i.e, unclutter editor!)
//==================================================

namespace di = boost::di;
namespace chrono = std::chrono;
namespace this_thread = std::this_thread;

using namespace std::chrono_literals;

using std::string;
using std::stringstream;
using std::size_t;
using std::move;
using std::bind;
using std::forward;
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
using std::getenv;
using std::thread;

using boost::optional;

using stateflag = std::atomic<bool>;

//==================================================
// Instance factory
//==================================================

namespace factory {
  template <class InstanceType, class... Deps>
  unique_ptr<InstanceType> generic_instance(Deps... deps) {
    return make_unique<InstanceType>(deps...);
  }

  template <class InstanceType, class... Deps>
  shared_ptr<InstanceType> generic_singleton(Deps... deps) {
    static auto instance = make_shared<InstanceType>(deps...);
    return instance;
  }
}

struct null_deleter {
  template <typename T>
  void operator()(T*) const {}
};

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

//==================================================
// Various tools and helpers functions
//==================================================

auto has_env = [](const char* var) { return getenv(var) != nullptr; };
auto read_env = [](const char* var, string&& fallback = "") {
  const char* value{getenv(var)};
  return value != nullptr ? value : fallback;
};

LEMONBUDDY_NS_END
