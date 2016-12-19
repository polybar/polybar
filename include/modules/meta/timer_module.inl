POLYBAR_NS

namespace modules {
  // public {{{

  template <typename Impl>
  void timer_module<Impl>::start() {
    CAST_MOD(Impl)->m_mainthread = thread(&timer_module::runner, this);
  }

  // }}}
  // protected {{{

  template <typename Impl>
  void timer_module<Impl>::runner() {
    try {
      while (CONST_MOD(Impl).running()) {
        {
          std::lock_guard<std::mutex> guard(this->m_updatelock);

          if (CAST_MOD(Impl)->update())
            CAST_MOD(Impl)->broadcast();
        }
        if (CONST_MOD(Impl).running()) {
          CAST_MOD(Impl)->sleep(m_interval);
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
