#pragma once

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

class signal_emitter;

namespace tags {
  enum class attribute;
  enum class controltag;
}  // namespace tags

enum class mousebtn;
struct bar_settings;
class logger;

class parser {
 public:
  using make_type = unique_ptr<parser>;
  static make_type make();

 public:
  explicit parser(signal_emitter& emitter, const logger& logger);
  void parse(const bar_settings& bar, string data);

 protected:
  void text(string&& data);
  void handle_action(mousebtn btn, bool closing, const string&& cmd);

 private:
  signal_emitter& m_sig;
  vector<mousebtn> m_actions;
  const logger& m_log;
};

POLYBAR_NS_END
