#include "components/parser.hpp"

#include <algorithm>

#include "components/types.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "settings.hpp"
#include "tags/parser.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/memory.hpp"
#include "utils/string.hpp"

POLYBAR_NS

using namespace signals::parser;

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
parser::make_type parser::make() {
  return factory_util::unique<parser>(signal_emitter::make(), logger::make());
}

/**
 * Construct parser instance
 */
parser::parser(signal_emitter& emitter, const logger& logger) : m_sig(emitter), m_log(logger) {}

/**
 * Process input string
 */
void parser::parse(const bar_settings& bar, string data) {
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
        case tags::tag_type::ALIGN:
          m_sig.emit(change_alignment{el.tag_data.subtype.align});
          break;
        case tags::tag_type::FORMAT:
          switch (el.tag_data.subtype.format) {
            case syntaxtag::A:
              handle_action(std::move(el));
              break;
            case syntaxtag::B:
              m_sig.emit(change_background{get_color(el.tag_data.color, bar.background)});
              break;
            case syntaxtag::F:
              m_sig.emit(change_foreground{get_color(el.tag_data.color, bar.foreground)});
              break;
            case syntaxtag::T:
              m_sig.emit(change_font{el.tag_data.font});
              break;
            case syntaxtag::O:
              m_sig.emit(offset_pixel{el.tag_data.offset});
              break;
            case syntaxtag::R:
              m_sig.emit(reverse_colors{});
              break;
            case syntaxtag::o:
              m_sig.emit(change_overline{get_color(el.tag_data.color, bar.overline.color)});
              break;
            case syntaxtag::u:
              m_sig.emit(change_underline{get_color(el.tag_data.color, bar.underline.color)});
              break;
            case syntaxtag::P:
              m_sig.emit(control{el.tag_data.ctrl});
              break;
            default:
              throw runtime_error(
                  "Unrecognized tag format: " + to_string(static_cast<int>(el.tag_data.subtype.format)));
          }
          break;
        case tags::tag_type::ATTR:
          attribute act = el.tag_data.attr;
          switch (el.tag_data.subtype.activation) {
            case attr_activation::ON:
              m_sig.emit(attribute_set{act});
              break;
            case attr_activation::OFF:
              m_sig.emit(attribute_unset{act});
              break;
            case attr_activation::TOGGLE:
              m_sig.emit(attribute_toggle{act});
              break;
            default:
              throw runtime_error(
                  "Unrecognized attribute activation: " + to_string(static_cast<int>(el.tag_data.subtype.activation)));
          }
          break;
      }
    } else {
      text(std::move(el.data));
    }
  }

  if (!m_actions.empty()) {
    throw unclosed_actionblocks(to_string(m_actions.size()) + " unclosed action block(s)");
  }
}

/**
 * Process text contents
 */
void parser::text(string&& data) {
#ifdef DEBUG_WHITESPACE
  string::size_type p;
  while ((p = data.find(' ')) != string::npos) {
    data.replace(p, 1, "-"s);
  }
#endif

  m_sig.emit(signals::parser::text{std::move(data)});
}

void parser::handle_action(tags::element&& el) {
  mousebtn btn = el.tag_data.action.btn;
  if (el.tag_data.action.closing) {
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
    m_sig.emit(action_begin{action{btn, std::move(el.data)}});
  }
}

POLYBAR_NS_END
