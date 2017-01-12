#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class event_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      CAST_MOD(Impl)->update();
      CAST_MOD(Impl)->broadcast();

      this->m_mainthread = thread(&event_module::runner, this);
    }

   protected:
    void runner() {
      try {
        while (this->running()) {
          CAST_MOD(Impl)->idle();

          if (!this->running()) {
            break;
          }

          std::lock_guard<std::mutex> guard(this->m_updatelock);

          if (!CAST_MOD(Impl)->has_event()) {
            continue;
          } else if (!this->running()) {
            break;
          } else if (!CAST_MOD(Impl)->update()) {
            continue;
          }

          CAST_MOD(Impl)->broadcast();
        }
      } catch (const exception& err) {
        CAST_MOD(Impl)->halt(err.what());
      }
    }
  };
}

POLYBAR_NS_END
