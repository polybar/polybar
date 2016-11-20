#pragma once

#include "components/builder.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class inotify_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start();

   protected:
    void runner();
    void watch(string path, int mask = IN_ALL_EVENTS);
    void idle();
    void poll_events();

   private:
    map<string, int> m_watchlist;
  };
}

POLYBAR_NS_END
