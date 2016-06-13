#include "bar.hpp"
#include "modules/script.hpp"
#include "utils/config.hpp"
#include "utils/io.hpp"
#include "utils/string.hpp"

using namespace modules;

ScriptModule::ScriptModule(const std::string& name_)
  : TimerModule(name_, 1s), builder(std::make_unique<Builder>(true)), counter(0)
{
  // Load configuration values {{{
  this->exec = config::get<std::string>(name(), "exec");
  this->tail = config::get<bool>(name(), "tail", this->tail);

  if (!this->tail)
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
}

bool ScriptModule::update()
{
  this->counter++;
  this->output.clear();

  try {
    std::string buf;

    auto execline = string::replace_all(this->exec, "%counter%", std::to_string(this->counter));
    auto command = std::make_unique<Command>("/usr/bin/env\nsh\n-c\n"+ execline);

    command->exec(false);

    while (!(buf = io::readline(command->get_stdout(PIPE_READ))).empty() || (this->tail && this->enabled())) {
      this->output.append(buf + "\n");

      if (this->tail) {
        this->broadcast();
        this->output.clear();
      }
    }

    command->wait();
  } catch (CommandException &e) {
    log_error(e.what());
  } catch (proc::ExecFailure &e) {
    log_error(e.what());
  }

  return !this->output.empty();
}

std::string ScriptModule::get_output()
{
  if (this->output.empty())
    return "";

  if (!this->click_left.empty())
    this->builder->cmd(Cmd::LEFT_CLICK, string::replace_all(this->click_left, "%counter%", std::to_string(this->counter)));
  if (!this->click_middle.empty())
    this->builder->cmd(Cmd::MIDDLE_CLICK, string::replace_all(this->click_middle, "%counter%", std::to_string(this->counter)));
  if (!this->click_right.empty())
    this->builder->cmd(Cmd::RIGHT_CLICK, string::replace_all(this->click_right, "%counter%", std::to_string(this->counter)));

  if (!this->scroll_up.empty())
    this->builder->cmd(Cmd::SCROLL_UP, string::replace_all(this->scroll_up, "%counter%", std::to_string(this->counter)));
  if (!scroll_down.empty())
    this->builder->cmd(Cmd::SCROLL_DOWN, string::replace_all(this->scroll_down, "%counter%", std::to_string(this->counter)));

  this->builder->node(this->Module::get_output());

  return this->builder->flush();
}

bool ScriptModule::build(Builder *builder, const std::string& tag)
{
  if (tag == TAG_OUTPUT)
    builder->node(string::replace_all(this->output, "\n", ""));
  else
    return false;

  return true;
}
