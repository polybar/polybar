#pragma once

#include <stdlib.h>
#include <mutex>

#include "common.hpp"

#include <iostream>
POLYBAR_NS

namespace env_util {
  bool has(const string& var);
  string get(const string& var, const string& fallback = "");

  namespace details {
    class env_mutex_holder {
     private:
      static std::mutex s_env_mutex;
      template <typename Fun>
      friend class env_guard_impl;
    };

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

    template <typename Fun>
    class env_guard_impl : std::lock_guard<std::mutex> {
     public:
      env_guard_impl(string name, const string& value, Fun cleanup)
          : std::lock_guard<std::mutex>(details::env_mutex_holder::s_env_mutex)
          , m_name(move(name))
          , m_value_empty(value.empty())
          , m_original_env()
          , cleanup(move(cleanup)) {
        m_original_env = get(m_name);

        if (!m_value_empty) {
          ::setenv(m_name.c_str(), value.c_str(), true);
        }
      }

      ~env_guard_impl() {
        if (!m_value_empty) {
          if (m_original_env.empty()) {
            ::unsetenv(m_name.c_str());
          } else {
            ::setenv(m_name.c_str(), m_original_env.c_str(), true);
          }
        }

        cleanup(m_value_empty);
      }

     private:
      string m_name;
      bool m_value_empty;
      string m_original_env;
      Fun cleanup;
    };

  }  // namespace details
#pragma GCC diagnostic pop

  template <typename Fun>
  class env_guard : details::env_guard_impl<Fun> {
   public:
    using details::env_guard_impl<Fun>::env_guard_impl;
  };
}  // namespace env_util

POLYBAR_NS_END
