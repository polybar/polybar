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
  static rgba get_color(tags::color_value c, rgba fallback) {
    if (c.type == tags::color_type::RESET) {
      return fallback;
    } else {
      return c.val;
    }
  }

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
  void dispatch::parse(const bar_settings& bar, string data) {
    tags::parser p;
    p.set(std::move(data));

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
                m_sig.emit(change_background{get_color(el.tag_data.color, bar.background)});
                break;
              case tags::syntaxtag::F:
                m_sig.emit(change_foreground{get_color(el.tag_data.color, bar.foreground)});
                break;
              case tags::syntaxtag::T:
                m_sig.emit(change_font{el.tag_data.font});
                break;
              case tags::syntaxtag::O:
                m_sig.emit(offset_pixel{el.tag_data.offset});
                break;
              case tags::syntaxtag::R:
                m_sig.emit(reverse_colors{});
                break;
              case tags::syntaxtag::o:
                m_sig.emit(change_overline{get_color(el.tag_data.color, bar.overline.color)});
                break;
              case tags::syntaxtag::u:
                m_sig.emit(change_underline{get_color(el.tag_data.color, bar.underline.color)});
                break;
              case tags::syntaxtag::P:
                m_sig.emit(control{el.tag_data.ctrl});
                break;
              case tags::syntaxtag::l:
                m_sig.emit(change_alignment{alignment::LEFT});
                break;
              case tags::syntaxtag::r:
                m_sig.emit(change_alignment{alignment::RIGHT});
                break;
              case tags::syntaxtag::c:
                m_sig.emit(change_alignment{alignment::CENTER});
                break;
              default:
                throw runtime_error(
                    "Unrecognized tag format: " + to_string(static_cast<int>(el.tag_data.subtype.format)));
            }
            break;
          case tags::tag_type::ATTR:
            tags::attribute act = el.tag_data.attr;
            switch (el.tag_data.subtype.activation) {
              case tags::attr_activation::ON:
                m_sig.emit(attribute_set{act});
                break;
              case tags::attr_activation::OFF:
                m_sig.emit(attribute_unset{act});
                break;
              case tags::attr_activation::TOGGLE:
                m_sig.emit(attribute_toggle{act});
                break;
              default:
                throw runtime_error("Unrecognized attribute activation: " +
                                    to_string(static_cast<int>(el.tag_data.subtype.activation)));
            }
            break;
        }
      } else {
        text(std::move(el.data));
      }
    }

    if (!m_actions.empty()) {
      throw runtime_error(to_string(m_actions.size()) + " unclosed action block(s)");
    }
  }

  /**
   * Process text contents
   */
  void dispatch::text(string&& data) {
#ifdef DEBUG_WHITESPACE
    string::size_type p;
    while ((p = data.find(' ')) != string::npos) {
      data.replace(p, 1, "-"s);
    }
#endif

    m_sig.emit(signals::parser::text{std::move(data)});
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
}  // namespace tags

POLYBAR_NS_END
