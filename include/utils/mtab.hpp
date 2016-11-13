#pragma once

#include <mntent.h>

#include "common.hpp"

LEMONBUDDY_NS

namespace mtab_util {
  /**
   * Wrapper for reading mtab entries
   */
  class reader {
   public:
    explicit reader() {
      if ((m_ptr = setmntent("/etc/mtab", "r")) == nullptr) {
        throw system_error("Failed to read mtab");
      }
    }

    ~reader() {
      if (m_ptr != nullptr) {
        endmntent(m_ptr);
      }
    }

    bool next(mntent** dst) {
      return (*dst = getmntent(m_ptr)) != nullptr;
    }

   protected:
    FILE* m_ptr = nullptr;
  };
}

LEMONBUDDY_NS_END
