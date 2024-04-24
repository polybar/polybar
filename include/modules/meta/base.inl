#pragma once
#include <cassert>

#include "components/builder.hpp"
#include "components/logger.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "modules/meta/base.hpp"
#include "utils/action_router.hpp"

POLYBAR_NS

namespace modules {

  // module<Impl> public {{{

  template <typename Impl>
  module<Impl>::module(const bar_settings& bar, string name, const config& conf)
      : m_sig(signal_emitter::make())
      , m_bar(bar)
      , m_log(logger::make())
      , m_conf(conf)
      , m_router(make_unique<action_router>())
      , m_name("module/" + name)
      , m_name_raw(name)
      , m_builder(make_unique<builder>(bar))
      , m_formatter(make_unique<module_formatter>(m_conf, m_name))
      , m_handle_events(m_conf.get(m_name, "handle-events", true))
      , m_visible(!m_conf.get(m_name, "hidden", false)) {
    m_router->register_action(EVENT_MODULE_TOGGLE, [this]() { action_module_toggle(); });
    m_router->register_action(EVENT_MODULE_SHOW, [this]() { action_module_show(); });
    m_router->register_action(EVENT_MODULE_HIDE, [this]() { action_module_hide(); });
  }

  template <typename Impl>
  module<Impl>::~module() noexcept {
    m_log.trace("%s: Deconstructing", name());

    if (running()) {
      /*
       * We can't stop in the destructor because we have to call the subclasses which at this point already have been
       * destructed.
       */
      m_log.err("%s: Module was not stopped before deconstructing.", name());
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

  template <class Impl>
  void module<Impl>::start() {
    m_enabled = true;
  }

  template <class Impl>
  void module<Impl>::join() {
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
    if (m_changed.exchange(false)) {
      m_log.info("%s: Rebuilding cache", name());
      m_cache = CAST_MOD(Impl)->get_output();
      // Make sure builder is really empty
      m_builder->flush();
      if (!m_cache.empty()) {
        // Add a reset tag after the module
        m_builder->control(tags::controltag::R);
        m_cache += m_builder->flush();
      }
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

    /*
     * Builder for building individual tags isolated, so that we can
     */
    builder tag_builder(m_bar);

    // Whether any tags have been processed yet
    bool has_tags = false;

    // Cursor pointing into 'value'
    size_t cursor = 0;
    const string& value{format->value};

    /*
     * Search for all tags in the format definition. A tag is enclosed in '<' and '>'.
     * Each tag is given to the module to produce some output for it. All other text is added as-is.
     */
    while (cursor < value.size()) {
      // Check if there are any tags left

      // Start index of next tag
      size_t start = value.find('<', cursor);

      if (start == string::npos) {
        break;
      }

      // End index (inclusive) of next tag
      size_t end = value.find('>', start + 1);

      if (end == string::npos) {
        break;
      }

      // Potential regular text that appears before the tag.
      string non_tag;

      // There is some non-tag text
      if (start > cursor) {
        /*
         * Produce anything between the previous and current tag as regular text.
         */
        non_tag = value.substr(cursor, start - cursor);
        if (!has_tags) {
          /*
           * If no module tag has been built we do not want to add
           * whitespace defined between the format tags, but we do still
           * want to output other non-tag content
           */
          non_tag = string_util::ltrim(move(non_tag), ' ');
        }
      }

      string tag = value.substr(start, end - start + 1);
      bool tag_built = CONST_MOD(Impl).build(&tag_builder, tag);
      string tag_content = tag_builder.flush();

      /*
       * Remove exactly one space between two tags if the second tag was not built.
       */
      if (!tag_built && has_tags && !format->spacing) {
        if (!non_tag.empty() && non_tag.back() == ' ') {
          non_tag.erase(non_tag.size() - 1);
        }
      }

      m_builder->node(non_tag);

      if (tag_built) {
        if (has_tags) {
          // format-spacing is added between all tags
          m_builder->spacing(format->spacing);
        }

        m_builder->node(tag_content);
        has_tags = true;
      }

      cursor = end + 1;
    }

    if (cursor < value.size()) {
      m_builder->node(value.substr(cursor));
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
} // namespace modules

POLYBAR_NS_END
