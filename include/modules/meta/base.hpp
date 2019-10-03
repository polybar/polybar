#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>

#include "common.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "utils/concurrency.hpp"
#include "utils/functional.hpp"
#include "utils/inotify.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace chrono = std::chrono;
using namespace std::chrono_literals;
using std::map;

#define DEFAULT_FORMAT "format"

#define DEFINE_MODULE(name, type) struct name : public type<name>
#define CONST_MOD(name) static_cast<name const&>(*this)
#define CAST_MOD(name) static_cast<name*>(this)

// fwd decl {{{

namespace drawtypes {
  class label;
  using label_t = shared_ptr<label>;
  class ramp;
  using ramp_t = shared_ptr<ramp>;
  class progressbar;
  using progressbar_t = shared_ptr<progressbar>;
  class animation;
  using animation_t = shared_ptr<animation>;
  class iconset;
  using iconset_t = shared_ptr<iconset>;
}

class builder;
class config;
class logger;
class signal_emitter;

// }}}

namespace modules {
  using namespace drawtypes;

  DEFINE_ERROR(module_error);
  DEFINE_CHILD_ERROR(undefined_format, module_error);
  DEFINE_CHILD_ERROR(undefined_format_tag, module_error);

  // class definition : module_format {{{

  struct module_format {
    string value{};
    vector<string> tags{};
    label_t prefix{};
    label_t suffix{};
    string fg{};
    string bg{};
    string ul{};
    string ol{};
    size_t ulsize{0};
    size_t olsize{0};
    size_t spacing{0};
    size_t padding{0};
    size_t margin{0};
    int offset{0};
    int font{0};

    string decorate(builder* builder, string output);
  };

  // }}}
  // class definition : module_formatter {{{

  class module_formatter {
   public:
    explicit module_formatter(const config& conf, string modname) : m_conf(conf), m_modname(modname) {}

    void add(string name, string fallback, vector<string>&& tags, vector<string>&& whitelist = {});
    bool has(const string& tag, const string& format_name);
    bool has(const string& tag);
    shared_ptr<module_format> get(const string& format_name);

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

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void halt(string error_message) = 0;
    virtual string contents() = 0;
  };

  // }}}
  // class definition : module {{{

  template <class Impl>
  class module : public module_interface {
   public:
    module(const bar_settings bar, string name);
    ~module() noexcept;

    string name() const;
    bool running() const;
    void stop();
    void halt(string error_message);
    void teardown();
    string contents();

   protected:
    void broadcast();
    void idle();
    void sleep(chrono::duration<double> duration);
    void wakeup();
    string get_format() const;
    string get_output();

   protected:
    signal_emitter& m_sig;
    const bar_settings m_bar;
    const logger& m_log;
    const config& m_conf;

    mutex m_buildlock;
    mutex m_updatelock;
    mutex m_sleeplock;
    std::condition_variable m_sleephandler;

    string m_name;
    unique_ptr<builder> m_builder;
    unique_ptr<module_formatter> m_formatter;
    vector<thread> m_threads;
    thread m_mainthread;

    bool m_handle_events{true};

   private:
    atomic<bool> m_enabled{true};
    atomic<bool> m_changed{true};
    string m_cache;
  };

  // }}}
}

POLYBAR_NS_END
