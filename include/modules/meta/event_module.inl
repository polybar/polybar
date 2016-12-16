POLYBAR_NS

namespace modules {
  // public {{{

  template <class Impl>
  void event_module<Impl>::start() {
    CAST_MOD(Impl)->m_mainthread = thread(&event_module::runner, this);
  }

  // }}}
  // protected {{{

  template <class Impl>
  void event_module<Impl>::runner() {
    try {
      // Send initial broadcast to warmup cache
      if (CONST_MOD(Impl).running()) {
        CAST_MOD(Impl)->update();
        CAST_MOD(Impl)->broadcast();
      }

      while (CONST_MOD(Impl).running()) {
        CAST_MOD(Impl)->idle();

        if (!CONST_MOD(Impl).running())
          break;

        std::lock_guard<std::mutex> guard(this->m_updatelock);
        {
          if (!CAST_MOD(Impl)->has_event())
            continue;
          if (!CONST_MOD(Impl).running())
            break;
          if (!CAST_MOD(Impl)->update())
            continue;
          CAST_MOD(Impl)->broadcast();
        }
      }
    } catch (const module_error& err) {
      CAST_MOD(Impl)->halt(err.what());
    } catch (const std::exception& err) {
      CAST_MOD(Impl)->halt(err.what());
    }
  }

  // }}}
}

POLYBAR_NS_END
