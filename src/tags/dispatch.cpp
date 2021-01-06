#include "tags/dispatch.hpp"

#include <algorithm>

#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "settings.hpp"
#include "tags/parser.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

using namespace signals::parser;

namespace tags {
  /**
   * Create instance
   */
  dispatch::make_type dispatch::make() {
    return factory_util::unique<dispatch>(signal_emitter::make(), logger::make());
  }

  /**
   * Construct parser instance
   */
  dispatch::dispatch(signal_emitter& emitter, const logger& logger) : m_sig(emitter), m_log(logger) {}

  /**
   * Process input string
   */
  void dispatch::parse(const bar_settings& bar, renderer_interface& renderer, string data) {
    tags::parser p;
    p.set(std::move(data));

    context ctxt(bar);

    while (p.has_next_element()) {
      tags::element el;
      try {
        el = p.next_element();
      } catch (const tags::error& e) {
        m_log.err("Parser error (reason: %s)", e.what());
        continue;
      }

      if (el.is_tag) {
        switch (el.tag_data.type) {
          case tags::tag_type::FORMAT:
            switch (el.tag_data.subtype.format) {
              case tags::syntaxtag::A:
                handle_action(el.tag_data.action.btn, el.tag_data.action.closing, std::move(el.data));
                break;
              case tags::syntaxtag::B:
                ctxt.apply_bg(el.tag_data.color);
                break;
              case tags::syntaxtag::F:
                ctxt.apply_fg(el.tag_data.color);
                break;
              case tags::syntaxtag::T:
                ctxt.apply_font(el.tag_data.font);
                break;
              case tags::syntaxtag::O:
                renderer.render_offset(ctxt, el.tag_data.offset);
                break;
              case tags::syntaxtag::R:
                ctxt.apply_reverse();
                break;
              case tags::syntaxtag::o:
                ctxt.apply_ol(el.tag_data.color);
                break;
              case tags::syntaxtag::u:
                ctxt.apply_ul(el.tag_data.color);
                break;
              case tags::syntaxtag::P:
                handle_control(ctxt, el.tag_data.ctrl);
                break;
              case tags::syntaxtag::l:
                ctxt.apply_alignment(alignment::LEFT);
                m_sig.emit(change_alignment{alignment::LEFT});
                break;
              case tags::syntaxtag::r:
                ctxt.apply_alignment(alignment::RIGHT);
                m_sig.emit(change_alignment{alignment::RIGHT});
                break;
              case tags::syntaxtag::c:
                ctxt.apply_alignment(alignment::CENTER);
                m_sig.emit(change_alignment{alignment::CENTER});
                break;
              default:
                throw runtime_error(
                    "Unrecognized tag format: " + to_string(static_cast<int>(el.tag_data.subtype.format)));
            }
            break;
          case tags::tag_type::ATTR:
            ctxt.apply_attr(el.tag_data.subtype.activation, el.tag_data.attr);
            break;
        }
      } else {
        handle_text(renderer, ctxt, std::move(el.data));
      }
    }

    if (!m_actions.empty()) {
      throw runtime_error(to_string(m_actions.size()) + " unclosed action block(s)");
    }
  }

  /**
   * Process text contents
   */
  void dispatch::handle_text(renderer_interface& renderer, context& ctxt, string&& data) {
#ifdef DEBUG_WHITESPACE
    string::size_type p;
    while ((p = data.find(' ')) != string::npos) {
      data.replace(p, 1, "-"s);
    }
#endif

    renderer.render_text(ctxt, std::move(data));
  }

  void dispatch::handle_action(mousebtn btn, bool closing, const string&& cmd) {
    if (closing) {
      if (btn == mousebtn::NONE) {
        if (!m_actions.empty()) {
          btn = m_actions.back();
          m_actions.pop_back();
        } else {
          m_log.err("parser: Closing action tag without matching tag");
        }
      } else {
        auto it = std::find(m_actions.crbegin(), m_actions.crend(), btn);

        if (it == m_actions.rend()) {
          m_log.err("parser: Closing action tag for button %d without matching opening tag", static_cast<int>(btn));
        } else {
          /*
           * We can't erase with a reverse iterator, we have to get
           * the forward iterator first
           * https://stackoverflow.com/a/1830240/5363071
           */
          m_actions.erase(std::next(it).base());
        }
      }
      m_sig.emit(action_end{btn});
    } else {
      m_actions.push_back(btn);
      m_sig.emit(action_begin{action{btn, std::move(cmd)}});
    }
  }

  void dispatch::handle_control(context& ctxt, controltag ctrl) {
    switch (ctrl) {
      case controltag::R:
        ctxt.reset();
        break;
      default:
        throw runtime_error("Unrecognized polybar control tag: " + to_string(static_cast<int>(ctrl)));
    }
  }

}  // namespace tags

POLYBAR_NS_END
