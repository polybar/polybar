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
#include "utils/config.hpp"
#include "utils/string.hpp"
#include "utils/concurrency.hpp"

using namespace std::chrono_literals;

#define Concat(one, two) one ## two

#define _Stringify(expr) #expr
#define Stringify(expr) _Stringify(expr)

#define DefineModule(ModuleName, ModuleType) struct ModuleName : public ModuleType<ModuleName>
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
  std::map<std::string, std::unique_ptr<Format>> formats;

  public:
    explicit ModuleFormatter(std::string module_name)
      : module_name(module_name) {}

    void add(std::string name, std::string fallback, std::vector<std::string>&& tags, std::vector<std::string>&& whitelist = {});
    std::unique_ptr<Format>& get(std::string format_name);
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

  template<typename ModuleImpl>
  class Module : public ModuleInterface
  {
    concurrency::Atomic<bool> enabled_flag;
    concurrency::Value<std::string> cache;

    protected:
      concurrency::SpinLock output_lock;
      concurrency::SpinLock broadcast_lock;

      std::mutex sleep_lock;
      std::condition_variable sleep_handler;

      std::string name_;
      std::unique_ptr<Builder> builder;
      std::unique_ptr<ModuleFormatter> formatter;
      std::vector<std::thread> threads;

    public:
      Module(const std::string& name, bool lazy_builder = true)
        : name_("module/"+ name), builder(std::make_unique<Builder>(lazy_builder))
      {
        this->enable(false);
        this->cache = "";
        this->formatter = std::make_unique<ModuleFormatter>(ConstCastModule(ModuleImpl).name());
      }

      ~Module()
      {
        if (this->enabled())
          this->stop();

        std::lock_guard<concurrency::SpinLock> lck(this->broadcast_lock);

        for (auto &&t : this->threads) {
          if (t.joinable())
            t.join();
          else
            log_warning("["+ ConstCastModule(ModuleImpl).name() +"] Runner thread not joinable");
        }

        log_trace(name());
      }

      std::string name() const {
        return name_;
      }

      void stop()
      {
        log_trace(name());
        this->wakeup();
        std::lock_guard<concurrency::SpinLock> lck(this->broadcast_lock);
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
        broadcast_module_update(ConstCastModule(ModuleImpl).name());
      }

      void sleep(std::chrono::duration<double> sleep_duration)
      {
        std::unique_lock<std::mutex> lck(this->sleep_lock);
        std::thread sleep_thread([&]{
          auto start = std::chrono::system_clock::now();
          while ((std::chrono::system_clock::now() - start) < sleep_duration) {
            std::this_thread::sleep_for(50ms);
          }
          this->wakeup();
        });
        sleep_thread.detach();
        this->sleep_handler.wait(lck);
      }

      void wakeup()
      {
        log_trace("Releasing sleep lock for "+ this->name_);
        this->sleep_handler.notify_one();
      }

      std::string get_format() {
        return DEFAULT_FORMAT;
      }

      std::string get_output()
      {
        std::lock_guard<concurrency::SpinLock> lck(this->output_lock);

        log_trace(ConstCastModule(ModuleImpl).name());

        if (!this->enabled()) {
          log_trace(ConstCastModule(ModuleImpl).name() +" is disabled");
          return "";
        }

        auto format_name = CastModule(ModuleImpl)->get_format();
        auto &&format = this->formatter->get(format_name);

        int i = 0;
        for (auto tag : string::split(format->value, ' ')) {
          if ((i > 0 && !tag.empty()) || tag.empty()) {
            this->builder->space(format->spacing);
          }

          if (tag[0] == '<' && tag[tag.length()-1] == '>') {
            if (!CastModule(ModuleImpl)->build(this->builder.get(), tag)) {
              this->builder->remove_trailing_space(format->spacing);
            }
          } else {
            this->builder->node(tag);
          }

          i++;
        }

        return format->decorate(this->builder.get(), this->builder->flush());
      }
  };

  template<typename ModuleImpl>
  class StaticModule : public Module<ModuleImpl>
  {
    using Module<ModuleImpl>::Module;

    public:
      void start()
      {
        this->enable(true);
        this->threads.emplace_back(std::thread(&StaticModule::broadcast, this));
      }

      bool build(Builder *builder, std::string tag)
      {
        return true;
      }
  };

  template<typename ModuleImpl>
  class TimerModule : public Module<ModuleImpl>
  {
    protected:
      std::chrono::duration<double> interval = 1s;

      concurrency::SpinLock update_lock;

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

  template<typename ModuleImpl>
  class EventModule : public Module<ModuleImpl>
  {
    using Module<ModuleImpl>::Module;

    protected:
      concurrency::SpinLock update_lock;

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

  template<typename ModuleImpl>
  class InotifyModule : public Module<ModuleImpl>
  {
    using Module<ModuleImpl>::Module;

    protected:
      std::map<std::string, int> watch_list;

      concurrency::SpinLock update_lock;

      void runner()
      {
        // warmup
        if (CastModule(ModuleImpl)->on_event(nullptr))
          CastModule(ModuleImpl)->broadcast();

        while (this->enabled()) {
          try {
            this->poll_events();
          } catch (InotifyException &e) {
            get_logger()->fatal(e.what());
          }
        }
      }

      void idle() const
      {
        // std::this_thread::sleep_for(1s);
      }

      void poll_events()
      {
        std::lock_guard<concurrency::SpinLock> lck(this->update_lock);
        std::vector<std::unique_ptr<InotifyWatch>> watches;

        try {
          for (auto &&w : this->watch_list)
            watches.emplace_back(std::make_unique<InotifyWatch>(w.first, w.second));
        } catch (InotifyException &e) {
          watches.clear();
          get_logger()->error(e.what());
          std::this_thread::sleep_for(100ms);
          return;
        }

        while (this->enabled()) {
          ConstCastModule(ModuleImpl).idle();

          for (auto &&w : watches) {
            log_trace("Polling inotify event for watch at "+ (*w)());

            if (w->has_event(500 / watches.size())) {
              std::unique_ptr<InotifyEvent> event = w->get_event();

              watches.clear();

              if (CastModule(ModuleImpl)->on_event(event.get()))
                CastModule(ModuleImpl)->broadcast();

              return;
            }
          }
        }
      }

      void watch(const std::string& path, int mask = InotifyEvent::ALL)
      {
        log_trace(path);
        this->watch_list.insert(std::make_pair(path, mask));
      }

    public:
      InotifyModule(const std::string& name, const std::string& path, int mask = InotifyEvent::ALL) : Module<ModuleImpl>(name)
      {
        this->watch(path, mask);
      }

      InotifyModule(const std::string& name, std::vector<std::string> paths, int mask = InotifyEvent::ALL) : Module<ModuleImpl>(name)
      {
        for (auto &&path : paths)
          this->watch(path, mask);
      }

      void start()
      {
        this->enable(true);
        this->threads.emplace_back(std::thread(&InotifyModule::runner, this));
      }
  };
}
