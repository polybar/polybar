#pragma once

#ifdef DEBUG
#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2
#endif

#include <boost/di.hpp>
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

POLYBAR_NS_END
