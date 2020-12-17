#pragma once

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

class signal_emitter;

enum class mousebtn;
struct bar_settings;
class logger;

namespace tags {
  enum class attribute;
  enum class controltag;

  /**
   * Calls into the tag parser to parse the given formatting string and then
   * sends the right signals for each tag.
   */
  class dispatch {
   public:
    using make_type = unique_ptr<dispatch>;
    static make_type make();

    explicit dispatch(signal_emitter& emitter, const logger& logger);
    void parse(const bar_settings& bar, string data);

   protected:
    void text(string&& data);
    void handle_action(mousebtn btn, bool closing, const string&& cmd);

   private:
    signal_emitter& m_sig;
    vector<mousebtn> m_actions;
    const logger& m_log;
  };
}  // namespace tags

POLYBAR_NS_END
