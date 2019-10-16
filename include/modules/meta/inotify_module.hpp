#pragma once

#include "components/builder.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

namespace modules {
  /**
   * A module that listen to inotify events.
   *
   * \details
   * To implement this module, the following method should be implemented:
   *   - #on_event(inotify_event*): CRTP implementation
   *
   * Optionally, this function can be reimplemented:
   *   - #poll_events(): CRTP implementation
   * \see module
   */
  template <class Impl>
  class inotify_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      this->m_mainthread = thread(&inotify_module::runner, this);
    }

   protected:
    void runner() {
      this->m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));
      try {
        // Warm up module output before entering the loop
        {
          std::lock_guard<std::mutex> guard(this->m_modulelock);
          CAST_MOD(Impl)->on_event(nullptr);
          CAST_MOD(Impl)->broadcast();
        }

        while (this->running()) {
          CAST_MOD(Impl)->poll_events();
        }
      } catch (const module_error& err) {
        CAST_MOD(Impl)->halt(err.what());
      } catch (const std::exception& err) {
        CAST_MOD(Impl)->halt(err.what());
      }
    }

    void watch(string path, int mask = IN_ALL_EVENTS) {
      std::lock_guard<std::mutex> guard(this->m_modulelock);
      this->m_log.trace("%s: Attach inotify at %s", this->name(), path);
      m_watchlist.insert(make_pair(path, mask));
    }

    void idle() {
      this->sleep(200ms);
    }

    /**
     * \brief This method should poll inotify events and dispatch them with the #on_event(inotify_event*) method.
     * \details
     * This method is NOT protected, it must lock the #m_modulelock mutex when modifying or accessing the attributes.
     * The mutex MUST also be locked before calling #on_event(inotify_event*) since it's a precondition to call this
     * method. Moreover the call to #on_event(inotify_event*) should be a CRTP call.
     */
    void poll_events() {
      vector<unique_ptr<inotify_watch>> watches;

      try {
        std::lock_guard<std::mutex> guard(this->m_modulelock);
        for (auto&& w : m_watchlist) {
          watches.emplace_back(inotify_util::make_watch(w.first));
          watches.back()->attach(w.second);
        }
      } catch (const system_error& e) {
        watches.clear();
        this->m_log.err("%s: Error while creating inotify watch (what: %s)", this->name(), e.what());
        CAST_MOD(Impl)->sleep(0.1s);
        return;
      }

      while (this->running()) {
        for (auto&& w : watches) {
          this->m_log.trace_x("%s: Poll inotify watch %s", this->name(), w->path());

          if (w->poll(1000 / watches.size())) {
            auto event = w->get_event();

            for (auto&& w : watches) {
              w->remove(true);
            }

            {
              std::lock_guard<std::mutex> guard(this->m_modulelock);
              if (CAST_MOD(Impl)->on_event(event.get())) {
                CAST_MOD(Impl)->broadcast();
              }
            }
            CAST_MOD(Impl)->idle();
            return;
          }

          if (!this->running())
            break;
        }
        CAST_MOD(Impl)->idle();
      }
    }

    /**
     * \brief Processes the given event
     * \details
     * This method must be implemented in subclasses.
     * It's CRTP called by poll_events() when an event is polled.
     * Contract:
     *   - expects: the mutex #m_modulelock is locked
     *   - ensures: the mutex #m_modulelock is still locked.
     *
     * This method shouldn't block.
     *
     * \param event - may be nullptr
     */
    void on_event(inotify_event* event) = delete;

   private:
    map<string, int> m_watchlist;
  };
}  // namespace modules

POLYBAR_NS_END
