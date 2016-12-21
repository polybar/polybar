#pragma once

#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class event_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      this->m_mainthread = thread(&event_module::runner, this);
    }

   protected:
    void runner() {
      try {
        // Warm up module output and
        // send broadcast before entering
        // the update loop
        if (CONST_MOD(Impl).running()) {
          CAST_MOD(Impl)->update();
          CAST_MOD(Impl)->broadcast();
        }

        while (CONST_MOD(Impl).running()) {
          CAST_MOD(Impl)->idle();

          if (!CONST_MOD(Impl).running()) {
            break;
          }

          std::lock_guard<std::mutex> guard(this->m_updatelock);

          if (!CAST_MOD(Impl)->has_event()) {
            continue;
          } else if (!CONST_MOD(Impl).running()) {
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
