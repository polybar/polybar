#pragma once

#include <algorithm>
#include <condition_variable>
#include <fastdelegate/fastdelegate.hpp>
#include <mutex>

#include "common.hpp"
#include "components/builder.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "utils/inotify.hpp"
#include "utils/string.hpp"
#include "utils/threading.hpp"

LEMONBUDDY_NS

#define DEFAULT_FORMAT "format"

#define DEFINE_MODULE(name, type) struct name : public type<name>
#define CONST_MOD(name) static_cast<name const&>(*this)
#define CAST_MOD(name) static_cast<name*>(this)

namespace modules {
  using namespace drawtypes;

  DEFINE_ERROR(module_error);
  DEFINE_CHILD_ERROR(undefined_format, module_error);
  DEFINE_CHILD_ERROR(undefined_format_tag, module_error);

  // class definition : module_format {{{

  struct module_format {
    string value;
    vector<string> tags;
    string fg;
    string bg;
    string ul;
    string ol;
    int spacing;
    int padding;
    int margin;
    int offset;

    string decorate(builder* builder, string output) {
      if (offset != 0)
        builder->offset(offset);
      if (margin > 0)
        builder->space(margin);
      if (!bg.empty())
        builder->background(bg);
      if (!fg.empty())
        builder->color(fg);
      if (!ul.empty())
        builder->underline(ul);
      if (!ol.empty())
        builder->overline(ol);
      if (padding > 0)
        builder->space(padding);

      builder->append(output);

      if (padding > 0)
        builder->space(padding);
      if (!ol.empty())
        builder->overline_close();
      if (!ul.empty())
        builder->underline_close();
      if (!fg.empty())
        builder->color_close();
      if (!bg.empty())
        builder->background_close();
      if (margin > 0)
        builder->space(margin);

      return builder->flush();
    }
  };

  // }}}
  // class definition : module_formatter {{{

  class module_formatter {
   public:
    explicit module_formatter(const config& conf, string modname)
        : m_conf(conf), m_modname(modname) {}

    void add(string name, string fallback, vector<string>&& tags, vector<string>&& whitelist = {}) {
      auto format = make_unique<module_format>();

      format->value = m_conf.get<string>(m_modname, name, fallback);
      format->fg = m_conf.get<string>(m_modname, name + "-foreground", "");
      format->bg = m_conf.get<string>(m_modname, name + "-background", "");
      format->ul = m_conf.get<string>(m_modname, name + "-underline", "");
      format->ol = m_conf.get<string>(m_modname, name + "-overline", "");
      format->spacing = m_conf.get<int>(m_modname, name + "-spacing", DEFAULT_SPACING);
      format->padding = m_conf.get<int>(m_modname, name + "-padding", 0);
      format->margin = m_conf.get<int>(m_modname, name + "-margin", 0);
      format->offset = m_conf.get<int>(m_modname, name + "-offset", 0);
      format->tags.swap(tags);

      for (auto&& tag : string_util::split(format->value, ' ')) {
        if (tag[0] != '<' || tag[tag.length() - 1] != '>')
          continue;
        if (std::find(format->tags.begin(), format->tags.end(), tag) != format->tags.end())
          continue;
        if (std::find(whitelist.begin(), whitelist.end(), tag) != whitelist.end())
          continue;
        throw undefined_format_tag("[" + m_modname + "] Undefined \"" + name + "\" tag: " + tag);
      }

      m_formats.insert(make_pair(name, std::move(format)));
    }

    shared_ptr<module_format> get(string format_name) {
      auto format = m_formats.find(format_name);
      if (format == m_formats.end())
        throw undefined_format("Format \"" + format_name + "\" has not been added");
      return format->second;
    }

    bool has(string tag, string format_name) {
      auto format = m_formats.find(format_name);
      if (format == m_formats.end())
        throw undefined_format(format_name.c_str());
      return format->second->value.find(tag) != string::npos;
    }

    bool has(string tag) {
      for (auto&& format : m_formats)
        if (format.second->value.find(tag) != string::npos)
          return true;
      return false;
    }

   protected:
    const config& m_conf;
    string m_modname;
    map<string, shared_ptr<module_format>> m_formats;
  };

  // }}}

  // class definition : module_interface {{{

  struct module_interface {
   public:
    virtual ~module_interface() {}

    virtual string name() const = 0;
    virtual bool running() const = 0;

    virtual void setup() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual string contents() = 0;

    virtual bool handle_event(string cmd) = 0;
    virtual bool receive_events() const = 0;

    virtual void set_writer(delegate::Delegate1<string>&& fn) = 0;
    virtual void set_terminator(delegate::Delegate1<string>&& fn) = 0;
  };

  // }}}
  // class definition : module {{{

  template <class Impl>
  class module : public module_interface {
   public:
    module(const bar_settings bar, const logger& logger, const config& config, string name)
        : m_bar(bar)
        , m_log(logger)
        , m_conf(config)
        , m_name("module/" + name)
        , m_builder(make_unique<builder>(bar))
        , m_formatter(make_unique<module_formatter>(m_conf, m_name)) {}

    ~module() {
      CAST_MOD(Impl)->stop();

      m_updatelock.unlock();

      std::lock_guard<threading_util::spin_lock> lck(m_updatelock);
      {
        if (m_broadcast_thread.joinable())
          m_broadcast_thread.join();

        for (auto&& thread_ : m_threads) {
          if (thread_.joinable())
            thread_.join();
        }

        m_threads.clear();
      }

      m_log.trace("%s: Done cleaning up", name());
    }

    void set_writer(delegate::Delegate1<string>&& fn) {
      m_writer = forward<decltype(fn)>(fn);
    }

    void set_terminator(delegate::Delegate1<string>&& fn) {
      m_terminator = forward<decltype(fn)>(fn);
    }

    string name() const {
      return m_name;
    }

    bool running() const {
      return CONST_MOD(Impl).enabled();
    }

    void setup() {
      m_log.trace("%s: Setup", name());
      CAST_MOD(Impl)->setup();
    }

    void stop() {
      if (!enabled())
        return;

      std::unique_lock<threading_util::spin_lock> lck(m_updatelock);
      {
        enable(false);
        CAST_MOD(Impl)->teardown();
        m_log.trace("%s: Stop", name());
      }

      if (m_terminator)
        m_terminator(name());
    }

    void teardown() {
      CAST_MOD(Impl)->wakeup();
    }

    string contents() {
      return m_cache;
    }

    bool handle_event(string cmd) {
      return CAST_MOD(Impl)->handle_event(cmd);
    }

    bool receive_events() const {
      return false;
    }

   protected:
    bool enabled() const {
      return m_enabled;
    }

    void enable(bool state) {
      m_enabled = state;
    }

    void broadcast() {
      if (!enabled())
        return;
      else if (!m_writer)
        m_log.warn("%s: No attach writer, ignoring broadcast...", name());

      m_cache = CAST_MOD(Impl)->get_output();
      m_writer(name());
    }

    void idle() {}

    void sleep(chrono::duration<double> sleep_duration) {
      std::unique_lock<std::mutex> lck(m_sleeplock);
      m_sleephandler.wait_for(lck, sleep_duration);
    }

    void wakeup() {
      m_log.trace("%s: Release sleep lock", name());
      // std::unique_lock<std::mutex> lck(m_sleeplock);
      m_sleephandler.notify_all();
    }

    string get_format() const {
      return DEFAULT_FORMAT;
    }

    string get_output() {
      if (!enabled()) {
        m_log.trace("%s: Module is disabled", name());
        return "";
      }

      auto format_name = CONST_MOD(Impl).get_format();
      auto format = m_formatter->get(format_name);

      int i = 0;
      bool tag_built = true;

      for (auto tag : string_util::split(format->value, ' ')) {
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

   protected:
    delegate::Delegate1<string> m_writer;
    delegate::Delegate1<string> m_terminator;

    threading_util::spin_lock m_updatelock;
    // std::timed_mutex m_mutex;

    const bar_settings m_bar;
    const logger& m_log;
    const config& m_conf;

    std::mutex m_sleeplock;
    std::condition_variable m_sleephandler;

    string m_name;
    unique_ptr<builder> m_builder;
    unique_ptr<module_formatter> m_formatter;
    vector<thread> m_threads;

   private:
    stateflag m_enabled{false};
    string m_cache;
    thread m_broadcast_thread;
  };

  // }}}

  // class definition : static_module {{{

  template <class Impl>
  class static_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      CAST_MOD(Impl)->setup();
      CAST_MOD(Impl)->enable(true);
      CAST_MOD(Impl)->broadcast();
    }

    bool build(builder*, string) const {
      return true;
    }
  };

  // }}}
  // class definition : timer_module {{{

  using interval_t = chrono::duration<double>;

  template <class Impl>
  class timer_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      CAST_MOD(Impl)->enable(true);
      CAST_MOD(Impl)->m_threads.emplace_back(thread(&timer_module::runner, this));
    }

   protected:
    interval_t m_interval = 1s;

    void runner() {
      try {
        CAST_MOD(Impl)->setup();

        while (CONST_MOD(Impl).enabled()) {
          {
            std::lock_guard<threading_util::spin_lock> lck(this->m_updatelock);
            if (CAST_MOD(Impl)->update())
              CAST_MOD(Impl)->broadcast();
          }
          CAST_MOD(Impl)->sleep(m_interval);
        }
      } catch (const std::exception& err) {
        this->m_log.err("%s: %s", CONST_MOD(Impl).name(), err.what());
        this->m_log.warn("Stopping '%s'...", CONST_MOD(Impl).name());
        CAST_MOD(Impl)->stop();
      }
    }
  };

  // }}}
  // class definition : event_module {{{

  template <class Impl>
  class event_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      CAST_MOD(Impl)->enable(true);
      CAST_MOD(Impl)->m_threads.emplace_back(thread(&event_module::runner, this));
    }

   protected:
    void runner() {
      try {
        CAST_MOD(Impl)->setup();

        // warmup
        CAST_MOD(Impl)->update();
        CAST_MOD(Impl)->broadcast();

        while (CONST_MOD(Impl).enabled()) {
          std::lock_guard<threading_util::spin_lock> lck(this->m_updatelock);

          if (!CAST_MOD(Impl)->has_event())
            CAST_MOD(Impl)->idle();
          else if (!CAST_MOD(Impl)->update())
            CAST_MOD(Impl)->idle();
          else
            CAST_MOD(Impl)->broadcast();
        }
      } catch (const std::exception& err) {
        this->m_log.err("%s: %s", CONST_MOD(Impl).name(), err.what());
        this->m_log.warn("Stopping '%s'...", CONST_MOD(Impl).name());
        CAST_MOD(Impl)->stop();
      }
    }
  };

  // }}}
  // class definition : inotify_module {{{

  template <class Impl>
  class inotify_module : public module<Impl> {
   public:
    using module<Impl>::module;

    void start() {
      CAST_MOD(Impl)->enable(true);
      CAST_MOD(Impl)->m_threads.emplace_back(thread(&inotify_module::runner, this));
    }

   protected:
    void runner() {
      try {
        CAST_MOD(Impl)->setup();
        CAST_MOD(Impl)->on_event(nullptr);  // warmup
        CAST_MOD(Impl)->broadcast();

        while (CAST_MOD(Impl)->enabled()) {
          CAST_MOD(Impl)->poll_events();
        }
      } catch (const std::exception& err) {
        this->m_log.err("%s: %s", CONST_MOD(Impl).name(), err.what());
        this->m_log.warn("Stopping '%s'...", CONST_MOD(Impl).name());
        CAST_MOD(Impl)->stop();
      }
    }

    void watch(string path, int mask = IN_ALL_EVENTS) {
      this->m_log.trace("%s: Attach inotify at %s", CONST_MOD(Impl).name(), path);
      m_watchlist.insert(make_pair(path, mask));
    }

    void idle() {
      CAST_MOD(Impl)->sleep(200ms);
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
        this->m_log.err(
            "%s: Error while creating inotify watch (what: %s)", CONST_MOD(Impl).name(), e.what());
        CAST_MOD(Impl)->sleep(0.1s);
        return;
      }

      while (CONST_MOD(Impl).enabled()) {
        for (auto&& w : watches) {
          this->m_log.trace("%s: Poll inotify watch %s", CONST_MOD(Impl).name(), w->path());
          std::lock_guard<threading_util::spin_lock> lck(this->m_updatelock);

          if (w->poll(1000 / watches.size())) {
            auto event = w->get_event();

            w->remove();

            if (CAST_MOD(Impl)->on_event(event.get()))
              CAST_MOD(Impl)->broadcast();

            return;
          }
        }
        CAST_MOD(Impl)->idle();
      }
    }

   private:
    map<string, int> m_watchlist;
  };

  // }}}
}

LEMONBUDDY_NS_END
