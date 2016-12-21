#pragma once

#include "components/builder.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class inotify_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      this->m_mainthread = thread(&inotify_module::runner, this);
    }

   protected:
    void runner() {
      try {
        // Warm up module output and
        // send broadcast before entering
        // the update loop
        if (CONST_MOD(Impl).running()) {
          CAST_MOD(Impl)->on_event(nullptr);
          CAST_MOD(Impl)->broadcast();
        }

        while (CONST_MOD(Impl).running()) {
          CAST_MOD(Impl)->poll_events();
        }
      } catch (const module_error& err) {
        CAST_MOD(Impl)->halt(err.what());
      } catch (const std::exception& err) {
        CAST_MOD(Impl)->halt(err.what());
      }
    }

    void watch(string path, int mask = IN_ALL_EVENTS) {
      this->m_log.trace("%s: Attach inotify at %s", CONST_MOD(Impl).name(), path);
      m_watchlist.insert(make_pair(path, mask));
    }

    void idle() {
      this->sleep(200ms);
    }

    void poll_events() {
      vector<unique_ptr<inotify_watch>> watches;

      try {
        for (auto&& w : m_watchlist) {
          watches.emplace_back(inotify_util::make_watch(w.first));
          watches.back()->attach(w.second);
        }
      } catch (const system_error& e) {
        watches.clear();
        this->m_log.err("%s: Error while creating inotify watch (what: %s)", CONST_MOD(Impl).name(), e.what());
        CAST_MOD(Impl)->sleep(0.1s);
        return;
      }

      while (CONST_MOD(Impl).running()) {
        std::unique_lock<std::mutex> guard(this->m_updatelock);
        {
          for (auto&& w : watches) {
            this->m_log.trace_x("%s: Poll inotify watch %s", CONST_MOD(Impl).name(), w->path());

            if (w->poll(1000 / watches.size())) {
              auto event = w->get_event();

              for (auto&& w : watches) {
                try {
                  w->remove();
                } catch (const system_error&) {
                }
              }

              if (CAST_MOD(Impl)->on_event(event.get()))
                CAST_MOD(Impl)->broadcast();

              CAST_MOD(Impl)->idle();

              return;
            }

            if (!CONST_MOD(Impl).running())
              break;
          }
        }
        guard.unlock();
        CAST_MOD(Impl)->idle();
      }
    }

   private:
    map<string, int> m_watchlist;
  };
}

POLYBAR_NS_END
