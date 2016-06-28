#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <map>
#include <thread>
#include <algorithm>

#include "exception.hpp"
#include "services/builder.hpp"
#include "services/inotify.hpp"
#include "services/logger.hpp"
#include "services/event_throttler.hpp"
#include "utils/config.hpp"
#include "utils/string.hpp"
#include "utils/concurrency.hpp"

using namespace std::chrono_literals;

#define Concat(one, two) one ## two

#define _Stringify(expr) #expr
#define Stringify(expr) _Stringify(expr)

#define DefineModule(ModuleName, ModuleType) struct ModuleName : public ModuleType<ModuleName>
#define DefineModule2(ModuleName, ModuleType, P2) struct ModuleName : public ModuleType<ModuleName, P2>
#define DefineModule3(ModuleName, ModuleType, P2, P3) struct ModuleName : public ModuleType<ModuleName, P2, P3>
#define CastModule(ModuleName) static_cast<ModuleName *>(this)
#define ConstCastModule(ModuleName) static_cast<ModuleName const &>(*this)

#define DEFAULT_FORMAT "format"

DefineBaseException(ModuleError);
DefineChildException(UndefinedFormat, ModuleError);
DefineChildException(UndefinedFormatTag, ModuleError);

class ModuleFormatter
{
  public:
    struct Format
    {
      std::string value;
      std::vector<std::string> tags;
      std::string fg, bg, ul, ol;
      int spacing, padding, margin, offset;

      std::string decorate(Builder *builder, std::string output);
    };

  std::string module_name;
  std::map<std::string, std::shared_ptr<Format>> formats;

  public:
    explicit ModuleFormatter(std::string module_name)
      : module_name(module_name) {}

    void add(std::string name, std::string fallback, std::vector<std::string>&& tags, std::vector<std::string>&& whitelist = {});
    std::shared_ptr<Format> get(std::string format_name);
    bool has(std::string tag, std::string format_name);
    bool has(std::string tag);
};

namespace modules
{
  void broadcast_module_update(std::string module_name);
  std::string get_tag_name(std::string tag);

  struct ModuleInterface
  {
    public:
      virtual ~ModuleInterface(){}

      virtual std::string name() const = 0;

      virtual void start() = 0;
      virtual void stop() = 0;
      virtual void refresh() = 0;

      virtual std::string operator()() = 0;

      virtual bool handle_command(std::string cmd) = 0;
  };

  template<class ModuleImpl>
  class Module : public ModuleInterface
  {
    concurrency::Atomic<bool> enabled_flag { false };
    concurrency::Value<std::string> cache { "" };

    protected:
      concurrency::SpinLock output_lock;
      concurrency::SpinLock broadcast_lock;
      concurrency::SpinLock update_lock;

      std::mutex sleep_lock;
      std::condition_variable sleep_handler;

      std::string name_;
      std::shared_ptr<Logger> logger;
      std::unique_ptr<Builder> builder;
      std::unique_ptr<EventThrottler> broadcast_throttler;
      std::unique_ptr<ModuleFormatter> formatter;
      std::vector<std::thread> threads;

      event_throttler::limit_t broadcast_throttler_limit() const {
        return event_throttler::limit_t(1);
      }

      event_throttler::timewindow_t broadcast_throttler_timewindow() const {
        return event_throttler::timewindow_t(25);
      }

    public:
      Module(std::string name, bool lazy_builder = true)
        : name_("module/"+ name)
        , logger(get_logger())
        , builder(std::make_unique<Builder>(lazy_builder))
        , broadcast_throttler(std::make_unique<EventThrottler>(ConstCastModule(ModuleImpl).broadcast_throttler_limit(), ConstCastModule(ModuleImpl).broadcast_throttler_timewindow()))
        , formatter(std::make_unique<ModuleFormatter>(ConstCastModule(ModuleImpl).name())) {}

      ~Module()
      {
        if (this->enabled())
          this->stop();

        std::lock_guard<concurrency::SpinLock> lck(this->update_lock);
        {
          for (auto &&t : this->threads) {
            if (t.joinable())
              t.join();
            else
              this->logger->warning("["+ ConstCastModule(ModuleImpl).name() +"] Runner thread not joinable");
          }

          this->threads.clear();
        }

        log_trace2(this->logger, name());
      }

      std::string name() const {
        return name_;
      }

      void stop()
      {
        std::lock_guard<concurrency::SpinLock> lck(this->broadcast_lock);
        log_trace2(this->logger, name());
        this->wakeup();
        this->enable(false);
      }

      void refresh() {
        this->cache = CastModule(ModuleImpl)->get_output();
      }

      std::string operator()() {
        return this->cache();
      }

      bool handle_command(std::string cmd) {
        return CastModule(ModuleImpl)->handle_command(cmd);
      }

    protected:
      bool enabled() {
        return this->enabled_flag();
      }

      void enable(bool state) {
        this->enabled_flag = state;
      }

      void broadcast()
      {
        std::lock_guard<concurrency::SpinLock> lck(this->broadcast_lock);
        if (!this->broadcast_throttler->passthrough()) {
          log_trace2(this->logger, "Throttled broadcast for: "+ this->name_);
          return;
        }
        broadcast_module_update(ConstCastModule(ModuleImpl).name());
      }

      void sleep(std::chrono::duration<double> sleep_duration)
      {
        std::unique_lock<std::mutex> lck(this->sleep_lock);
        this->sleep_handler.wait_for(lck, sleep_duration);
      }

      void wakeup()
      {
        log_trace2(this->logger, "Releasing sleep lock for "+ this->name_);
        this->sleep_handler.notify_all();
      }

      std::string get_format() {
        return DEFAULT_FORMAT;
      }

      std::string get_output()
      {
        std::lock_guard<concurrency::SpinLock> lck(this->output_lock);

        if (!this->enabled()) {
          log_trace2(this->logger, ConstCastModule(ModuleImpl).name() +" is disabled");
          return "";
        } else {
          log_trace2(this->logger, ConstCastModule(ModuleImpl).name());
        }

        auto format_name = CastModule(ModuleImpl)->get_format();
        auto format = this->formatter->get(format_name);

        int i = 0;
        bool tag_built = true;

        for (auto tag : string::split(format->value, ' ')) {
          bool is_blankspace = tag.empty();

          if (tag[0] == '<' && tag[tag.length()-1] == '>') {
            if (i > 0)
              this->builder->space(format->spacing);
            if (!(tag_built = CastModule(ModuleImpl)->build(this->builder.get(), tag)) && i > 0)
              this->builder->remove_trailing_space(format->spacing);
            if (tag_built)
              i++;
          } else if (is_blankspace && tag_built) {
            this->builder->node(" ");
          } else if (!is_blankspace) {
            this->builder->node(tag);
          }
        }

        return format->decorate(this->builder.get(), this->builder->flush());
      }
  };

  template<class ModuleImpl>
  class StaticModule : public Module<ModuleImpl>
  {
    using Module<ModuleImpl>::Module;

    public:
      void start()
      {
        this->enable(true);
        this->threads.emplace_back(std::thread(&StaticModule::broadcast, this));
      }

      bool build(Builder *builder, std::string tag) {
        return true;
      }
  };

  template<class ModuleImpl>
  class TimerModule : public Module<ModuleImpl>
  {
    protected:
      std::chrono::duration<double> interval = 1s;

      void runner()
      {
        while (this->enabled()) {
          { std::lock_guard<concurrency::SpinLock> lck(this->update_lock);
            if (CastModule(ModuleImpl)->update())
              CastModule(ModuleImpl)->broadcast();
          }
          this->sleep(this->interval);
        }
      }

    public:
      template<typename I>
      TimerModule(std::string name, I const &interval)
        : Module<ModuleImpl>(name), interval(interval) {}

      void start()
      {
        this->enable(true);
        this->threads.emplace_back(std::thread(&TimerModule::runner, this));
      }
  };

  template<class ModuleImpl>
  class EventModule : public Module<ModuleImpl>
  {
    using Module<ModuleImpl>::Module;

    protected:
      void runner()
      {
        // warmup
        CastModule(ModuleImpl)->update();
        CastModule(ModuleImpl)->broadcast();

        while (this->enabled()) {
          std::lock_guard<concurrency::SpinLock> lck(this->update_lock);

          if (!CastModule(ModuleImpl)->has_event())
            continue;

          if (!CastModule(ModuleImpl)->update())
            continue;

          CastModule(ModuleImpl)->broadcast();
        }
      }

    public:
      void start()
      {
        this->enable(true);
        this->threads.emplace_back(std::thread(&EventModule::runner, this));
      }
  };

  template<class ModuleImpl, const int ThrottleLimit = 2, const int ThrottleWindowMs = 50>
  class InotifyModule : public Module<ModuleImpl>
  {
    protected:
      concurrency::SpinLock poll_lock;

      std::unique_ptr<EventThrottler> poll_throttler;

      std::map<std::string, int> watch_list;

      void runner()
      {
        // warmup
        CastModule(ModuleImpl)->on_event(nullptr);
        CastModule(ModuleImpl)->broadcast();

        while (this->enabled()) {
          try {
            std::lock_guard<concurrency::SpinLock> lck(this->poll_lock);
            if (this->poll_throttler->passthrough())
              this->poll_events();
          } catch (InotifyException &e) {
            get_logger()->fatal(e.what());
          }
        }
      }

      void idle() const {
        // this->sleep(1s);
      }

      void poll_events()
      {
        std::vector<std::unique_ptr<InotifyWatch>> watches;

        try {
          for (auto &&w : this->watch_list)
            watches.emplace_back(std::make_unique<InotifyWatch>(w.first, w.second));
        } catch (InotifyException &e) {
          watches.clear();
          log_error(e.what());
          this->sleep(0.1s);
          return;
        }

        while (this->enabled()) {
          ConstCastModule(ModuleImpl).idle();

          for (auto &&w : watches) {
            log_trace2(this->logger, "Polling inotify event for watch at "+ (*w)());

            if (w->has_event(500 / watches.size())) {
              std::unique_ptr<InotifyEvent> event = w->get_event();

              watches.clear();

              std::lock_guard<concurrency::SpinLock> lck(this->update_lock);

              if (!this->poll_throttler->passthrough())
                return;

              if (CastModule(ModuleImpl)->on_event(event.get()))
                CastModule(ModuleImpl)->broadcast();

              return;
            }
          }
        }
      }

      void watch(std::string path, int mask = InotifyEvent::ALL)
      {
        log_trace2(this->logger, path);
        this->watch_list.insert(std::make_pair(path, mask));
      }

    public:
      explicit InotifyModule(std::string name)
        : Module<ModuleImpl>(name)
        , poll_throttler(std::make_unique<EventThrottler>(event_throttler::limit_t(ThrottleLimit), event_throttler::timewindow_t(ThrottleWindowMs))) {}

      InotifyModule(std::string name, std::string path, int mask = InotifyEvent::ALL)
        : Module<ModuleImpl>(name)
        , poll_throttler(std::make_unique<EventThrottler>(event_throttler::limit_t(ThrottleLimit), event_throttler::timewindow_t(ThrottleWindowMs))) {
        this->watch(path, mask);
      }

      InotifyModule(std::string name, std::vector<std::string> paths, int mask = InotifyEvent::ALL)
        : Module<ModuleImpl>(name)
        , poll_throttler(std::make_unique<EventThrottler>(event_throttler::limit_t(ThrottleLimit), event_throttler::timewindow_t(ThrottleWindowMs)))
      {
        for (auto &&path : paths)
          this->watch(path, mask);
      }

      ~InotifyModule() {
        std::lock_guard<concurrency::SpinLock> lck(this->poll_lock);
      }

      void start()
      {
        this->enable(true);
        this->threads.emplace_back(std::thread(&InotifyModule::runner, this));
      }
  };
}
