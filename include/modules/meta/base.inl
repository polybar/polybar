#include "components/builder.hpp"
#include "components/logger.hpp"
#include "components/config.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"

POLYBAR_NS

namespace sig_ev = signals::eventqueue;

namespace modules {
  // module<Impl> public {{{

  template <typename Impl>
  module<Impl>::module(const bar_settings bar, string name)
        : m_sig(signal_emitter::make())
        , m_bar(bar)
        , m_log(logger::make())
        , m_conf(config::make())
        , m_name("module/" + name)
        , m_builder(make_unique<builder>(bar))
        , m_formatter(make_unique<module_formatter>(m_conf, m_name)) {}

  template <typename Impl>
  module<Impl>::~module() noexcept {
    m_log.trace("%s: Deconstructing", name());

    for (auto&& thread_ : m_threads) {
      if (thread_.joinable()) {
        thread_.join();
      }
    }
    if (m_mainthread.joinable()) {
      m_mainthread.join();
    }
  }

  template <typename Impl>
  string module<Impl>::name() const {
    return m_name;
  }

  template <typename Impl>
  bool module<Impl>::running() const {
    return static_cast<bool>(m_enabled);
  }

  template <typename Impl>
  void module<Impl>::stop() {
    if (!running()) {
      return;
    }

    m_log.info("%s: Stopping", name());
    m_enabled = false;

    std::lock(m_buildlock, m_updatelock);
    std::lock_guard<std::mutex> guard_a(m_buildlock, std::adopt_lock);
    std::lock_guard<std::mutex> guard_b(m_updatelock, std::adopt_lock);
    {
      CAST_MOD(Impl)->wakeup();
      CAST_MOD(Impl)->teardown();

      m_sig.emit(sig_ev::process_check{});
    }
  }

  template <typename Impl>
  void module<Impl>::halt(string error_message) {
    m_log.err("%s: %s", name(), error_message);
    m_log.warn("Stopping '%s'...", name());
    stop();
  }

  template <typename Impl>
  void module<Impl>::teardown() {}

  template <typename Impl>
  string module<Impl>::contents() {
    return m_cache;
  }

  // }}}
  // module<Impl> protected {{{

  template <typename Impl>
  void module<Impl>::broadcast() {
    if (!running()) {
      return;
    }

    m_cache = CAST_MOD(Impl)->get_output();
    m_sig.emit(sig_ev::process_broadcast{});
  }

  template <typename Impl>
  void module<Impl>::idle() {
    CAST_MOD(Impl)->sleep(25ms);
  }

  template <typename Impl>
  void module<Impl>::sleep(chrono::duration<double> sleep_duration) {
    std::unique_lock<std::mutex> lck(m_sleeplock);
    m_sleephandler.wait_for(lck, sleep_duration);
  }

  template <typename Impl>
  void module<Impl>::wakeup() {
    m_log.trace("%s: Release sleep lock", name());
    m_sleephandler.notify_all();
  }

  template <typename Impl>
  string module<Impl>::get_format() const {
    return DEFAULT_FORMAT;
  }

  template <typename Impl>
  string module<Impl>::get_output() {
    if (!running()) {
      m_log.trace("%s: Module is disabled", name());
      return "";
    }
    std::lock_guard<std::mutex> guard(m_buildlock);

    auto format_name = CONST_MOD(Impl).get_format();
    auto format = m_formatter->get(format_name);

    int i = 0;
    bool tag_built = true;

    for (auto&& tag : string_util::split(format->value, ' ')) {
      bool is_blankspace = tag.empty();

      if (tag[0] == '<' && tag[tag.length() - 1] == '>') {
        if (i > 0)
          m_builder->space(format->spacing);
        if (!(tag_built = CONST_MOD(Impl).build(m_builder.get(), tag)) && i > 0)
          m_builder->remove_trailing_space(format->spacing);
        if (tag_built)
          i++;
      } else if (is_blankspace && tag_built) {
        m_builder->node(" ");
      } else if (!is_blankspace) {
        m_builder->node(tag);
      }
    }

    return format->decorate(m_builder.get(), m_builder->flush());
  }

  // }}}
}

POLYBAR_NS_END
