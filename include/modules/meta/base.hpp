#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>

#include "common.hpp"
#include "components/config.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "utils/concurrency.hpp"
#include "utils/inotify.hpp"
#include "utils/string.hpp"
POLYBAR_NS

namespace chrono = std::chrono;
using namespace std::chrono_literals;
using std::atomic;
using std::map;

#define DEFAULT_FORMAT "format"

#define CONST_MOD(name) static_cast<name const&>(*this)
#define CAST_MOD(name) static_cast<name*>(this)

// fwd decl {{{

namespace drawtypes {
  class ramp;
  using ramp_t = shared_ptr<ramp>;
  class progressbar;
  using progressbar_t = shared_ptr<progressbar>;
  class animation;
  using animation_t = shared_ptr<animation>;
  class iconset;
  using iconset_t = shared_ptr<iconset>;
} // namespace drawtypes

class builder;
class config;
class logger;
class signal_emitter;

class action_router;
// }}}

namespace modules {

  using namespace drawtypes;

  DEFINE_ERROR(module_error);
  DEFINE_CHILD_ERROR(undefined_format, module_error);
  DEFINE_CHILD_ERROR(undefined_format_tag, module_error);

  // class definition : module_format {{{

  struct module_format {
    string value{};
    label_t prefix{};
    label_t suffix{};
    rgba fg{};
    rgba bg{};
    rgba ul{};
    rgba ol{};
    size_t ulsize{0};
    size_t olsize{0};
    spacing_val spacing{ZERO_SPACE};
    spacing_val padding{ZERO_SPACE};
    spacing_val margin{ZERO_SPACE};
    extent_val offset{ZERO_PX_EXTENT};
    int font{0};

    string decorate(builder* builder, string output);
  };

  // }}}
  // class definition : module_formatter {{{

  class module_formatter {
   public:
    explicit module_formatter(const config& conf, string modname) : m_conf(conf), m_modname(modname) {}

    void add(string name, string fallback, vector<string>&& tags, vector<string>&& whitelist = {});
    void add_optional(string name, vector<string>&& tags, vector<string>&& whitelist = {});
    bool has(const string& tag, const string& format_name);
    bool has(const string& tag);
    bool has_format(const string& format_name);
    shared_ptr<module_format> get(const string& format_name);

   protected:
    void add_value(string&& name, string&& value, vector<string>&& tags, vector<string>&& whitelist);

    const config& m_conf;
    string m_modname;
    map<string, shared_ptr<module_format>> m_formats;
  };

  // }}}

  // class definition : module_interface {{{

  struct module_interface {
   public:
    virtual ~module_interface() {}

    /**
     * The type users have to specify in the module section `type` key
     */
    virtual string type() const = 0;

    /**
     * Module name w/o 'module/' prefix
     */
    virtual string name_raw() const = 0;
    virtual string name() const = 0;
    virtual bool running() const = 0;
    virtual bool visible() const = 0;

    /**
     * Handle action, possibly with data attached
     *
     * Any implementation is free to ignore the data, if the action does not
     * require additional data.
     *
     * @returns true if the action is supported and false otherwise
     */
    virtual bool input(const string& action, const string& data) = 0;

    virtual void start() = 0;
    virtual void join() = 0;
    virtual void stop() = 0;
    virtual void halt(string error_message) = 0;
    virtual string contents() = 0;
  };

  // }}}
  // class definition : module {{{

  template <class Impl>
  class module : public module_interface {
   public:
    module(const bar_settings& bar, string name, const config&);
    ~module() noexcept;

    static constexpr auto EVENT_MODULE_TOGGLE = "module_toggle";
    static constexpr auto EVENT_MODULE_SHOW = "module_show";
    static constexpr auto EVENT_MODULE_HIDE = "module_hide";

    string type() const override;

    string name_raw() const override;
    string name() const override;
    bool running() const override;

    bool visible() const override;

    void start() override;
    void join() final override;
    void stop() override;
    void halt(string error_message) override;
    void teardown();
    string contents() override;

    bool input(const string& action, const string& data) final override;

   protected:
    void broadcast();
    void idle();
    void sleep(chrono::duration<double> duration);
    template <class Clock, class Duration>
    void sleep_until(chrono::time_point<Clock, Duration> point);

    /**
     * Wakes up the module.
     *
     * It should be possible to interrupt any blocking operation inside a
     * module using this function.
     *
     * In addition, after a wake up whatever was woken up should immediately
     * check whether the module is still running.
     *
     * Modules that don't follow this, could stall the operation of whatever
     * code called this function.
     */
    void wakeup();
    string get_format() const;
    string get_output();

    virtual void set_visible(bool value);

    void action_module_toggle();
    void action_module_show();
    void action_module_hide();

   protected:
    signal_emitter& m_sig;
    const bar_settings& m_bar;
    const logger& m_log;
    const config& m_conf;

    unique_ptr<action_router> m_router;

    mutex m_buildlock;
    mutex m_updatelock;
    mutex m_sleeplock;
    std::condition_variable m_sleephandler;

    const string m_name;
    const string m_name_raw;
    unique_ptr<builder> m_builder;
    unique_ptr<module_formatter> m_formatter;
    vector<thread> m_threads;
    thread m_mainthread;

    bool m_handle_events{true};

   private:
    atomic<bool> m_enabled{false};
    atomic<bool> m_visible{true};
    atomic<bool> m_changed{true};
    string m_cache;
  };

  // }}}
} // namespace modules

POLYBAR_NS_END
