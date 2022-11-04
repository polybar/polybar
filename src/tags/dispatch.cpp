#include "tags/dispatch.hpp"

#include <algorithm>

#include "components/renderer.hpp"
#include "events/signal.hpp"
#include "settings.hpp"
#include "tags/parser.hpp"
#include "utils/color.hpp"
#include "components/logger.hpp"

POLYBAR_NS

namespace tags {
  /**
   * Create instance
   */
  dispatch::make_type dispatch::make(action_context& action_ctxt) {
    return std::make_unique<dispatch>(logger::make(), action_ctxt);
  }

  /**
   * Construct parser instance
   */
  dispatch::dispatch(const logger& logger, action_context& action_ctxt) : m_log(logger), m_action_ctxt(action_ctxt) {}

  /**
   * Process input string
   */
  void dispatch::parse(const bar_settings& bar, renderer_interface& renderer, const string&& data) {
    tags::parser p;
    p.set(std::move(data));

    m_action_ctxt.reset();
    m_ctxt = make_unique<context>(bar);

    while (p.has_next_element()) {
      tags::element el;
      try {
        el = p.next_element();
      } catch (const tags::error& e) {
        m_log.err("Parser error (reason: %s)", e.what());
        continue;
      }

      alignment old_alignment = m_ctxt->get_alignment();
      double old_x = old_alignment == alignment::NONE ? 0 : renderer.get_x(*m_ctxt);

      if (el.is_tag) {
        switch (el.tag_data.type) {
          case tags::tag_type::FORMAT:
            switch (el.tag_data.subtype.format) {
              case tags::syntaxtag::A:
                handle_action(renderer, el.tag_data.action.btn, el.tag_data.action.closing, std::move(el.data));
                break;
              case tags::syntaxtag::B:
                m_ctxt->apply_bg(el.tag_data.color);
                break;
              case tags::syntaxtag::F:
                m_ctxt->apply_fg(el.tag_data.color);
                break;
              case tags::syntaxtag::T:
                m_ctxt->apply_font(el.tag_data.font);
                break;
              case tags::syntaxtag::O:
                handle_offset(renderer, el.tag_data.offset);
                break;
              case tags::syntaxtag::R:
                m_ctxt->apply_reverse();
                break;
              case tags::syntaxtag::o:
                m_ctxt->apply_ol(el.tag_data.color);
                break;
              case tags::syntaxtag::u:
                m_ctxt->apply_ul(el.tag_data.color);
                break;
              case tags::syntaxtag::P:
                handle_control(renderer, el.tag_data.ctrl);
                break;
              case tags::syntaxtag::l:
                handle_alignment(renderer, alignment::LEFT);
                break;
              case tags::syntaxtag::r:
                handle_alignment(renderer, alignment::RIGHT);
                break;
              case tags::syntaxtag::c:
                handle_alignment(renderer, alignment::CENTER);
                break;
              default:
                throw runtime_error(
                    "Unrecognized tag format: " + to_string(static_cast<int>(el.tag_data.subtype.format)));
            }
            break;
          case tags::tag_type::ATTR:
            m_ctxt->apply_attr(el.tag_data.subtype.activation, el.tag_data.attr);
            break;
        }
      } else {
        handle_text(renderer, std::move(el.data));
      }

      if (old_alignment == m_ctxt->get_alignment()) {
        double new_x = renderer.get_x(*m_ctxt);
        if (new_x < old_x) {
          m_action_ctxt.compensate_for_negative_move(old_alignment, old_x, new_x);
        }
      }
    }

    /*
     * After rendering, we need to tell the action context about the position
     * of the alignment blocks so that it can do intersection tests.
     */
    for (auto a : {alignment::LEFT, alignment::CENTER, alignment::RIGHT}) {
      m_action_ctxt.set_alignment_start(a, renderer.get_alignment_start(a));
    }
    renderer.apply_tray_position(*m_ctxt);

    auto num_unclosed = m_action_ctxt.num_unclosed();

    if (num_unclosed != 0) {
      throw runtime_error(to_string(num_unclosed) + " unclosed action block(s)");
    }
  }

  /**
   * Process text contents
   */
  void dispatch::handle_text(renderer_interface& renderer, string&& data) {
#ifdef DEBUG_WHITESPACE
    string::size_type p;
    while ((p = data.find(' ')) != string::npos) {
      data.replace(p, 1, "-"s);
    }
#endif

    renderer.render_text(*m_ctxt, std::move(data));
  }

  void dispatch::handle_action(renderer_interface& renderer, mousebtn btn, bool closing, const string&& cmd) {
    if (closing) {
      m_action_ctxt.action_close(btn, m_ctxt->get_alignment(), renderer.get_x(*m_ctxt));
    } else {
      m_action_ctxt.action_open(btn, std::move(cmd), m_ctxt->get_alignment(), renderer.get_x(*m_ctxt));
    }
  }

  void dispatch::handle_offset(renderer_interface& renderer, extent_val offset) {
    renderer.render_offset(*m_ctxt, offset);
  }

  void dispatch::handle_alignment(renderer_interface& renderer, alignment a) {
    m_ctxt->apply_alignment(a);
    renderer.change_alignment(*m_ctxt);
  }

  void dispatch::handle_control(renderer_interface& renderer, controltag ctrl) {
    switch (ctrl) {
      case controltag::R:
        m_ctxt->apply_reset();
        break;
      case controltag::t:
        m_ctxt->store_tray_position(renderer.get_x(*m_ctxt));
        break;
      default:
        throw runtime_error("Unrecognized polybar control tag: " + to_string(static_cast<int>(ctrl)));
    }
  }

} // namespace tags

POLYBAR_NS_END
