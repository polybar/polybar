#ifndef POLYBAR_UDEV_MODULE_HPP
#define POLYBAR_UDEV_MODULE_HPP

#include "components/builder.hpp"
#include "modules/meta/base.hpp"
#include "utils/udev.hpp"

POLYBAR_NS

namespace modules {
  template <class Impl>
  class udev_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      this->m_mainthread = thread(&udev_module::runner, this);
    }
    void watch(string&& subsystem) {
      m_watch = udev_util::make_watch(move(subsystem));
    }

    void idle() {
      this->sleep(200ms);
    }

    void poll_events() {
      while (this->running()) {
        if (m_watch->poll()) {
          auto event = m_watch->get_event();

          if (CAST_MOD(Impl)->on_event(move(event))) {
            CAST_MOD(Impl)->broadcast();
          }

          CAST_MOD(Impl)->idle();
          return;
        }

        if (!this->running())
          break;

        CAST_MOD(Impl)->idle();
      }
    }

    void runner() {
      this->m_log.trace("%s: Thread id = %i", this->name(), concurrency_util::thread_id(this_thread::get_id()));
      m_watch->attach();
      try {
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

   private:
    unique_ptr<udev_watch> m_watch;
  };
}  // namespace modules

POLYBAR_NS_END

#endif  // POLYBAR_UDEV_MODULE_HPP
