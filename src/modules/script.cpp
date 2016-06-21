#include "bar.hpp"
#include "modules/script.hpp"
#include "utils/config.hpp"
#include "utils/io.hpp"
#include "utils/string.hpp"

using namespace modules;

ScriptModule::ScriptModule(std::string name_)
  : TimerModule(name_, 1s), builder(std::make_unique<Builder>(true)), counter(0)
{
  // Load configuration values {{{
  this->exec = config::get<std::string>(name(), "exec");
  this->tail = config::get<bool>(name(), "tail", this->tail);

  if (this->tail)
    this->interval = 0s;
  else
    this->interval = std::chrono::duration<double>(config::get<float>(name(), "interval", 1));

  this->click_left = config::get<std::string>(name(), "click-left", "");
  this->click_middle = config::get<std::string>(name(), "click-middle", "");
  this->click_right = config::get<std::string>(name(), "click-right", "");

  this->scroll_up = config::get<std::string>(name(), "scroll-up", "");
  this->scroll_down = config::get<std::string>(name(), "scroll-down", "");
  // }}}

  // Add formats and elements {{{
  this->formatter->add(DEFAULT_FORMAT, TAG_OUTPUT, { TAG_OUTPUT });
  // }}}

  this->counter = 0;
}

void ScriptModule::start()
{
  this->TimerModule::start();

  if (!this->tail)
    return;

  // Start the tail script in a separate thread {{{
  this->threads.push_back(std::thread([&]{
    while (this->enabled() && (!this->command || !this->command->is_running())) {
      log_debug("Executing command: "+ this->exec);
      this->counter++;
      this->command = std::make_unique<Command>("/usr/bin/env\nsh\n-c\n"
          + string::replace_all(this->exec, "%counter%", std::to_string(this->counter)));
      this->command->exec(true);
    }
  }));
  // }}}
}

bool ScriptModule::update()
{
  int bytes_read;

  if (this->tail) {
    if (!this->command)
      return false;
    if (!io::poll_read(this->command->get_stdout(PIPE_READ), 100))
      return false;
    this->output = io::readline(this->command->get_stdout(PIPE_READ), bytes_read);
    return bytes_read > 0;
  }

  try {
    this->counter++;
    this->output.clear();

    this->command = std::make_unique<Command>("/usr/bin/env\nsh\n-c\n"
        + string::replace_all(this->exec, "%counter%", std::to_string(this->counter)));
    this->command->exec(false);

    while (true) {
      auto buf = io::readline(this->command->get_stdout(PIPE_READ), bytes_read);
      if (bytes_read <= 0)
        break;
      this->output.append(buf +"\n");
    }

    if (this->command)
      this->command->wait();
  } catch (CommandException &e) {
    log_error(e.what());
  } catch (proc::ExecFailure &e) {
    log_error(e.what());
  }

  return true;
}

std::string ScriptModule::get_output()
{
  if (this->output.empty())
    return "";

  auto counter_str = std::to_string(this->counter);

  if (!this->click_left.empty())
    this->builder->cmd(Cmd::LEFT_CLICK, string::replace_all(this->click_left, "%counter%", counter_str));
  if (!this->click_middle.empty())
    this->builder->cmd(Cmd::MIDDLE_CLICK, string::replace_all(this->click_middle, "%counter%", counter_str));
  if (!this->click_right.empty())
    this->builder->cmd(Cmd::RIGHT_CLICK, string::replace_all(this->click_right, "%counter%", counter_str));

  if (!this->scroll_up.empty())
    this->builder->cmd(Cmd::SCROLL_UP, string::replace_all(this->scroll_up, "%counter%", counter_str));
  if (!scroll_down.empty())
    this->builder->cmd(Cmd::SCROLL_DOWN, string::replace_all(this->scroll_down, "%counter%", counter_str));

  this->builder->node(this->Module::get_output());

  return this->builder->flush();
}

bool ScriptModule::build(Builder *builder, std::string tag)
{
  if (tag == TAG_OUTPUT)
    builder->node(string::replace_all(this->output, "\n", ""));
  else
    return false;

  return true;
}
