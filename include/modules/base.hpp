#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <shared_mutex>
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

      std::string decorate(Builder *builder, const std::string& output)
      {
        if (this->offset != 0) builder->offset(this->offset);

        if (this->margin > 0) builder->space(this->margin);

        if (!this->bg.empty()) builder->background(this->bg);
        if (!this->fg.empty()) builder->color(this->fg);
        if (!this->ul.empty()) builder->underline(this->ul);
        if (!this->ol.empty()) builder->overline(this->ol);

        if (this->padding > 0) builder->space(this->padding);

        builder->append(output);

        if (this->padding > 0) builder->space(this->padding);

        if (!this->ol.empty()) builder->overline_close();
        if (!this->ul.empty()) builder->underline_close();
        if (!this->fg.empty()) builder->color_close();
        if (!this->bg.empty()) builder->background_close();

        if (this->margin > 0) builder->space(this->margin);

        return builder->flush();
      }
    };

  std::string module_name;
  std::map<std::string, std::unique_ptr<Format>> formats;

  public:
    explicit ModuleFormatter(const std::string& module_name)
      : module_name(module_name) {}

    void add(const std::string& name, const std::string& fallback, std::vector<std::string> &&tags, std::vector<std::string> &&whitelist = {})
    {
      auto format = std::make_unique<Format>();

      format->value = config::get<std::string>(this->module_name, name, fallback);
      format->fg = config::get<std::string>(this->module_name, name +":foreground", "");
      format->bg = config::get<std::string>(this->module_name, name +":background", "");
      format->ul = config::get<std::string>(this->module_name, name +":underline", "");
      format->ol = config::get<std::string>(this->module_name, name +":overline", "");
      format->spacing = config::get<int>(this->module_name, name +":spacing", DEFAULT_SPACING);
      format->padding = config::get<int>(this->module_name, name +":padding", 0);
      format->margin = config::get<int>(this->module_name, name +":margin", 0);
      format->offset = config::get<int>(this->module_name, name +":offset", 0);
      format->tags.swap(tags);

      for (auto &&tag : string::split(format->value, ' ')) {
        if (tag[0] != '<' || tag[tag.length()-1] != '>')
          continue;
        if (std::find(format->tags.begin(), format->tags.end(), tag) != format->tags.end())
          continue;
        if (std::find(whitelist.begin(), whitelist.end(), tag) != whitelist.end())
          continue;
        throw UndefinedFormatTag("["+ this->module_name +"] Undefined \""+ name +"\" tag: "+ tag);
      }

      this->formats.insert(std::make_pair(name, std::move(format)));
    }

    std::unique_ptr<Format>& get(const std::string& format_name)
    {
      auto format = this->formats.find(format_name);
      if (format == this->formats.end())
        throw UndefinedFormat("Format \""+ format_name +"\" has not been added");
      return format->second;
    }

    bool has(const std::string& tag, const std::string& format_name)
    {
      auto format = this->formats.find(format_name);
      if (format == this->formats.end())
        throw UndefinedFormat(format_name);
      return format->second->value.find(tag) != std::string::npos;
    }

    bool has(const std::string& tag)
    {
      for (auto &&format : this->formats)
        if (format.second->value.find(tag) != std::string::npos)
          return true;
      return false;
    }
};

namespace modules
{
  void broadcast_module_update(const std::string& module_name);
  std::string get_tag_name(const std::string& tag);

  struct ModuleInterface
  {
    public:
      virtual ~ModuleInterface(){}

      virtual std::string name() const = 0;

      virtual void start() = 0;
      virtual void stop() = 0;
      virtual void refresh() = 0;

      virtual std::string operator()() = 0;

      virtual bool handle_command(const std::string& cmd) = 0;
  };

  template<typename ModuleImpl>
  class Module : public ModuleInterface
  {
    concurrency::Atomic<bool> enabled_flag;
    concurrency::Value<std::string> cache;

    protected:
      concurrency::SpinLock output_lock;
      concurrency::SpinLock broadcast_lock;

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
        // this->builder = std::make_unique<Builder>(false);
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

      void stop() {
        log_trace(name());
        std::lock_guard<concurrency::SpinLock> lck(this->broadcast_lock);
        this->enable(false);
      }

      void refresh() {
        this->cache = CastModule(ModuleImpl)->get_output();
      }

      std::string operator()() {
        return this->cache();
      }

      bool handle_command(const std::string& cmd) {
        return CastModule(ModuleImpl)->handle_command(cmd);
      }

    protected:
      bool enabled() {
        return this->enabled_flag();
      }

      void enable(bool state) {
        this->enabled_flag = state;
      }

      void broadcast() {
        std::lock_guard<concurrency::SpinLock> lck(this->broadcast_lock);
        CastModule(ModuleImpl)->refresh();
        broadcast_module_update(ConstCastModule(ModuleImpl).name());
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

      bool build(Builder *builder, const std::string& tag)
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
          std::lock_guard<concurrency::SpinLock> lck(this->update_lock);
          if (CastModule(ModuleImpl)->update())
            CastModule(ModuleImpl)->broadcast();
          std::this_thread::sleep_for(this->interval);
        }
      }

    public:
      template<typename I>
      TimerModule(const std::string& name, I const &interval)
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

        for (auto &&w : this->watch_list)
          watches.emplace_back(std::make_unique<InotifyWatch>(w.first, w.second));

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
