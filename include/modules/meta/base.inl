#include <cassert>

#include "components/builder.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "modules/meta/base.hpp"
#include "utils/action_router.hpp"

POLYBAR_NS

namespace modules {

  // module<Impl> public {{{

  template <typename Impl>
  module<Impl>::module(const bar_settings bar, string name)
      : m_sig(signal_emitter::make())
      , m_bar(bar)
      , m_log(logger::make())
      , m_conf(config::make())
      , m_router(make_unique<action_router<Impl>>(CAST_MOD(Impl)))
      , m_name("module/" + name)
      , m_name_raw(name)
      , m_builder(make_unique<builder>(bar))
      , m_formatter(make_unique<module_formatter>(m_conf, m_name))
      , m_handle_events(m_conf.get(m_name, "handle-events", true))
      , m_visible(!m_conf.get(m_name, "hidden", false)) {
        m_router->register_action(EVENT_MODULE_TOGGLE, &module<Impl>::action_module_toggle);
        m_router->register_action(EVENT_MODULE_SHOW, &module<Impl>::action_module_show);
        m_router->register_action(EVENT_MODULE_HIDE, &module<Impl>::action_module_hide);
      }

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
  string module<Impl>::name_raw() const {
    return m_name_raw;
  }

  template <typename Impl>
  string module<Impl>::type() const {
    return string(Impl::TYPE);
  }

  template <typename Impl>
  bool module<Impl>::running() const {
    return static_cast<bool>(m_enabled);
  }

  template <typename Impl>
  bool module<Impl>::visible() const {
    return static_cast<bool>(m_visible);
  }

  template <typename Impl>
  void module<Impl>::stop() {
    if (!static_cast<bool>(m_enabled)) {
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

      m_sig.emit(signals::eventqueue::check_state{});
    }
  }

  template <typename Impl>
  void module<Impl>::halt(string error_message) {
    m_log.err("%s: %s", name(), error_message);
    m_log.notice("Stopping '%s'...", name());
    stop();
  }

  template <typename Impl>
  void module<Impl>::teardown() {}

  template <typename Impl>
  string module<Impl>::contents() {
    if (m_changed) {
      m_log.info("%s: Rebuilding cache", name());
      m_cache = CAST_MOD(Impl)->get_output();
      // Make sure builder is really empty
      m_builder->flush();
      if (!m_cache.empty()) {
        // Add a reset tag after the module
        m_builder->control(tags::controltag::R);
        m_cache += m_builder->flush();
      }
      m_changed = false;
    }
    return m_cache;
  }

  template <typename Impl>
  bool module<Impl>::input(const string& name, const string& data) {
    if (!m_router->has_action(name)) {
      return false;
    }

    try {
      m_router->invoke(name, data);
    } catch (const exception& err) {
      m_log.err("%s: Failed to handle command '%s' with data '%s' (%s)", this->name(), name, data, err.what());
    }
    return true;
  }

  // }}}
  // module<Impl> protected {{{

  template <typename Impl>
  void module<Impl>::broadcast() {
    m_changed = true;
    m_sig.emit(signals::eventqueue::notify_change{});
  }

  template <typename Impl>
  void module<Impl>::idle() {
    if (running()) {
      CAST_MOD(Impl)->sleep(25ms);
    }
  }

  template <typename Impl>
  void module<Impl>::sleep(chrono::duration<double> sleep_duration) {
    if (running()) {
      std::unique_lock<std::mutex> lck(m_sleeplock);
      m_sleephandler.wait_for(lck, sleep_duration);
    }
  }

  template <typename Impl>
  template <class Clock, class Duration>
  void module<Impl>::sleep_until(chrono::time_point<Clock, Duration> point) {
    if (running()) {
      std::unique_lock<std::mutex> lck(m_sleeplock);
      m_sleephandler.wait_until(lck, point);
    }
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
    std::lock_guard<std::mutex> guard(m_buildlock);
    auto format_name = CONST_MOD(Impl).get_format();
    auto format = m_formatter->get(format_name);
    bool no_tag_built{true};
    bool fake_no_tag_built{false};
    bool tag_built{false};
    auto mingap = std::max(1.f, format->spacing.value);
    size_t start, end;
    string value{format->value};
    while ((start = value.find('<')) != string::npos && (end = value.find('>', start)) != string::npos) {
      if (start > 0) {
        if (no_tag_built) {
          // If no module tag has been built we do not want to add
          // whitespace defined between the format tags, but we do still
          // want to output other non-tag content
          auto trimmed = string_util::ltrim(value.substr(0, start), ' ');
          if (!trimmed.empty()) {
            fake_no_tag_built = false;
            m_builder->node(move(trimmed));
          }
        } else {
          m_builder->node(value.substr(0, start));
        }
        value.erase(0, start);
        end -= start;
        start = 0;
      }
      string tag{value.substr(start, end + 1)};
      if (tag.empty()) {
        continue;
      } else if (tag[0] == '<' && tag[tag.size() - 1] == '>') {
        if (!no_tag_built)
          m_builder->spacing(format->spacing);
        else if (fake_no_tag_built)
          no_tag_built = false;
        if (!(tag_built = CONST_MOD(Impl).build(m_builder.get(), tag)) && !no_tag_built)
          m_builder->remove_trailing_space(mingap);
        if (tag_built)
          no_tag_built = false;
      }
      value.erase(0, tag.size());
    }

    if (!value.empty()) {
      m_builder->append(value);
    }

    return format->decorate(&*m_builder, m_builder->flush());
  }

  template <typename Impl>
  void module<Impl>::set_visible(bool value) {
    m_log.notice("%s: Visibility changed (state=%s)", m_name, value ? "shown" : "hidden");
    m_visible = value;
    broadcast();
  }

  template <typename Impl>
  void module<Impl>::action_module_toggle() {
    set_visible(!m_visible);
  }

  template <typename Impl>
  void module<Impl>::action_module_show() {
    set_visible(true);
  }

  template <typename Impl>
  void module<Impl>::action_module_hide() {
    set_visible(false);
  }

  // }}}
}  // namespace modules

POLYBAR_NS_END
