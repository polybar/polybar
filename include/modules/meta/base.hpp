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
}  // namespace drawtypes

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
    explicit module_formatter(const config& conf, string modname) : m_conf(conf), m_modname(move(modname)) {}

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
    virtual ~module_interface() = default;

    virtual string name() const = 0;
    virtual bool running() const = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void halt(string error_message) = 0;
    virtual string contents() = 0;
  };

  // }}}
  // class definition : module {{{

  /**
   * @brief Base classes of every modules.
   * @details
   * To implement a simple module, the following methods must be implemented:
   *   - #start(): virtual override
   *   - #build(builder*, const string&): CRTP implementation
   *
   * Optionally, the following methods might be reimplemented:
   *   - all the method of #module_interface excepts #contents(), #halt(string)
   *   - #wakeup(): CRTP implementation
   *   - #teardown(): CRTP implementation
   *   - #get_output(): CRTP implementation
   *   - #get_format(): CRTP implementation
   *
   * @tparam Impl - The final type of the module
   */
  template <class Impl>
  class module : public module_interface {
   public:
    module(const bar_settings bar, string name);
    ~module() noexcept;

    /**
     * @brief Returns the name of the module.
     * @details
     * This method call doesn't need to be protected.
     */
    string name() const final;

    /**
     * @brief Returns true if the module is running
     * @details
     * This method call doesn't need to be protected
     */
    bool running() const final;

    /**
     * @brief Stops the module
     * @details
     * If you need to clean the internal state of your module, #teardown should be used.
     *
     * This method does nothing if #running() returns false.
     * This method should call #wakeup() then #teardown() using CRTP.
     * These calls MUST be protected by locking #m_modulelock.
     */
    void stop() override;

    /**
     * @brief Logs an error and stop the module
     * @details
     * Logs the given error and call #stop()
     */
    void halt(string error_message) final;

    string contents() final;

   protected:
    /**
     * @brief Interrupts sleep
     * @details
     * This method is NOT protected
     */
    void wakeup();

    /**
     * @brief This method is called when the module is stopping.
     * @details
     * Contract:
     *   - expects: the mutex #m_modulelock is locked
     *   - ensures: the mutex #m_modulelock is still locked.
     */
    void teardown();

    /**
     * @brief Notifies the controller that an update of the bar is needed.
     */
    void broadcast();

    /**
     * @brief Action that should be executed when the module is idle.
     * @details
     * This method does nothing if #running() returns false.
     * Usually this method call #sleep(chrono::duration<double>).
     */
    void idle();

    /**
     * @brief Suspends the thread for the given duration.
     * @details
     * Waits on a condition variable. The sleep can be interrupted by using the #wakeup() method.
     */
    void sleep(chrono::duration<double> duration);

    /**
     * @brief Returns the current format of the module.
     * @details
     * Contract:
     *   - expects: the mutex #m_modulelock is locked
     *   - ensures: the mutex #m_modulelock is still locked.
     */
    string get_format() const;

    /**
     * @brief Computes and returns the output of the module.
     * @details
     * Contract:
     *   - expects: the mutex #m_modulelock is locked
     *   - ensures: the mutex #m_modulelock is still locked.
     *
     * This method is called by #contents() and shouldn't cache its output.
     * The caching is done by #contents()
     */
    string get_output();

    /**
     * @brief Adds part of the output corresponding to the tag.
     * @details
     * Contract:
     *   - expects: the mutex #m_modulelock is locked
     *   - ensures: the mutex #m_modulelock is still locked.
     *
     */
    bool build(builder* builder, const string& tag) const = delete;

   protected:
    signal_emitter& m_sig;
    const bar_settings m_bar;
    const logger& m_log;
    const config& m_conf;

    mutex m_modulelock;
    mutex m_sleeplock;
    std::condition_variable m_sleephandler;

    const string m_name;
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
}  // namespace modules

POLYBAR_NS_END
