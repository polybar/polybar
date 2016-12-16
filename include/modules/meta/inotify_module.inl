POLYBAR_NS

namespace modules {
  // public {{{

  template <class Impl>
  void inotify_module<Impl>::start() {
    CAST_MOD(Impl)->m_mainthread = thread(&inotify_module::runner, this);
  }

  // }}}
  // protected {{{

  template <class Impl>
  void inotify_module<Impl>::runner() {
    try {
      // Send initial broadcast to warmup cache
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

  template <class Impl>
  void inotify_module<Impl>::watch(string path, int mask) {
    this->m_log.trace("%s: Attach inotify at %s", CONST_MOD(Impl).name(), path);
    m_watchlist.insert(make_pair(path, mask));
  }

  template <class Impl>
  void inotify_module<Impl>::idle() {
    CAST_MOD(Impl)->sleep(200ms);
  }

  template <class Impl>
  void inotify_module<Impl>::poll_events() {
    vector<inotify_util::watch_t> watches;

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

  // }}}
}

POLYBAR_NS_END
