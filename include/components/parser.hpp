#pragma once

#include "common.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "tags/parser.hpp"
#include "utils/color.hpp"

POLYBAR_NS

class signal_emitter;
enum class attribute;
enum class controltag;
enum class mousebtn;
struct bar_settings;

DEFINE_ERROR(parser_error);
DEFINE_CHILD_ERROR(unrecognized_token, parser_error);
DEFINE_CHILD_ERROR(unrecognized_attribute, parser_error);
DEFINE_CHILD_ERROR(unclosed_actionblocks, parser_error);

class parser {
 public:
  using make_type = unique_ptr<parser>;
  static make_type make();

 public:
  explicit parser(signal_emitter& emitter, const logger& logger);
  void parse(const bar_settings& bar, string data);

 protected:
  void text(string&& data);
  void handle_action(tags::element&& e);

 private:
  signal_emitter& m_sig;
  vector<mousebtn> m_actions;
  const logger& m_log;
};

POLYBAR_NS_END
