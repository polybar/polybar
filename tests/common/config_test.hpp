#pragma once

#include <string>

#include "drawtypes/label.hpp"
#include "components/config_parser.hpp"

POLYBAR_NS

std::string get_text(label_t label) {
  label->reset_tokens();
  return label->get();
}

config::make_type load_config(std::string&& path, std::string&& bar) {
  logger log(loglevel::NONE);
  string config_txt = path;
  config_parser parser(log, move(config_txt), move(bar));
  return move(parser.parse());
}

POLYBAR_NS_END
