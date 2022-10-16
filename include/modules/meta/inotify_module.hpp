#pragma once

#include "components/builder.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class inotify_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() override {
      this->module<Impl>::start();
      this->m_mainthread = thread(&inotify_module::runner, this);
    }

   protected:
    void runner() {
      this->m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));
      try {
        // Warm up module output before entering the loop
        std::unique_lock<std::mutex> guard(this->m_updatelock);
        CAST_MOD(Impl)->on_event({});
        CAST_MOD(Impl)->broadcast();
        guard.unlock();

        while (this->running()) {
          std::lock_guard<std::mutex> guard(this->m_updatelock);
          CAST_MOD(Impl)->poll_events();
        }
      } catch (const module_error& err) {
        CAST_MOD(Impl)->halt(err.what());
      } catch (const std::exception& err) {
        CAST_MOD(Impl)->halt(err.what());
      }
    }

    void watch(string path, int mask = IN_ALL_EVENTS) {
      this->m_log.trace("%s: Attach inotify at %s", this->name(), path);
      m_watchlist.insert(make_pair(path, mask));
    }

    void idle() {
      this->sleep(200ms);
    }

    void poll_events() {
      vector<inotify_watch> watches;

      try {
        for (auto&& w : m_watchlist) {
          watches.emplace_back(w.first);
          watches.back().attach(w.second);
        }
      } catch (const system_error& e) {
        this->m_log.err("%s: Error while creating inotify watch (what: %s)", this->name(), e.what());
        CAST_MOD(Impl)->sleep(0.1s);
        return;
      }

      while (this->running()) {
        for (auto&& w : watches) {
          this->m_log.trace_x("%s: Poll inotify watch %s", this->name(), w.path());

          if (w.poll(1000 / watches.size())) {
            auto event = w.get_event();
            if (CAST_MOD(Impl)->on_event(event)) {
              CAST_MOD(Impl)->broadcast();
            }
            CAST_MOD(Impl)->idle();
            return;
          }

          if (!this->running()) {
            break;
          }
        }
        CAST_MOD(Impl)->idle();
      }
    }

   private:
    map<string, int> m_watchlist;
  };
} // namespace modules

POLYBAR_NS_END
