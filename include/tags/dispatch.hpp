#pragma once

#include "common.hpp"
#include "components/renderer_interface.hpp"
#include "components/types.hpp"
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
    static make_type make(action_context& action_ctxt);

    explicit dispatch(const logger& logger, action_context& action_ctxt);
    void parse(const bar_settings& bar, renderer_interface&, const string&& data);

   protected:
    void handle_text(renderer_interface& renderer, string&& data);
    void handle_action(renderer_interface& renderer, mousebtn btn, bool closing, const string&& cmd);
    void handle_offset(renderer_interface& renderer, extent_val offset);
    void handle_alignment(renderer_interface& renderer, alignment a);
    void handle_control(renderer_interface& renderer, controltag ctrl);

   private:
    const logger& m_log;

    unique_ptr<context> m_ctxt;
    action_context& m_action_ctxt;
  };
} // namespace tags

POLYBAR_NS_END
