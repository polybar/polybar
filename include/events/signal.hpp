#pragma once

#include "common.hpp"

#include "components/ipc.hpp"
#include "components/parser.hpp"
#include "components/types.hpp"
#include "events/types.hpp"
#include "utils/functional.hpp"

POLYBAR_NS

// fwd decl {{{

enum class mousebtn : uint8_t;
enum class syntaxtag : uint8_t;
enum class alignment : uint8_t;
enum class attribute : uint8_t;

// }}}

#define DEFINE_SIGNAL(id, name)                        \
  struct name : public detail::base_signal<name, id> { \
    using base_type::base_type;                        \
  }

#define DEFINE_VALUE_SIGNAL(id, name, type)                   \
  struct name : public detail::value_signal<name, id, type> { \
    using base_type::base_type;                               \
  }

namespace signals {
  namespace detail {
    // definition : signal {{{

    class signal {
     public:
      explicit signal() = default;
      virtual ~signal() {}
      virtual size_t size() const = 0;
    };

    // }}}
    // definition : base_signal {{{

    template <typename Derived, uint8_t Id>
    class base_signal : public signal {
     public:
      using base_type = base_signal<Derived, Id>;

      explicit base_signal() = default;

      virtual ~base_signal() {}

      static uint8_t id() {
        return Id;
      }

      virtual size_t size() const override {
        return sizeof(Derived);
      };
    };

    // }}}
    // definition : value_signal {{{

    template <typename Derived, uint8_t Id, typename ValueType>
    class value_signal : public base_signal<Derived, Id> {
     public:
      using base_type = value_signal<Derived, Id, ValueType>;
      using value_type = ValueType;

      explicit value_signal() {}
      explicit value_signal(ValueType&& data) {
        memcpy(m_data, &data, sizeof(data));
      }

      virtual ~value_signal() {}

      const ValueType* operator()() const {
        return reinterpret_cast<const ValueType*>(&m_data);
      }
      const ValueType* data() const {
        return reinterpret_cast<const ValueType*>(&m_data);
      }

     private:
      char m_data[sizeof(ValueType)];
    };

    // }}}
  }

  namespace eventqueue {
    DEFINE_VALUE_SIGNAL(1, process_quit, event);
    DEFINE_VALUE_SIGNAL(2, process_update, event);
    DEFINE_VALUE_SIGNAL(3, process_input, string);
    DEFINE_SIGNAL(4, process_check);
    DEFINE_SIGNAL(5, process_broadcast);
  }

  namespace ipc {
    DEFINE_VALUE_SIGNAL(20, process_command, ipc_command);
    DEFINE_VALUE_SIGNAL(21, process_hook, ipc_hook);
    DEFINE_VALUE_SIGNAL(22, process_action, ipc_action);
  }

  namespace ui {
    DEFINE_SIGNAL(50, tick);
    DEFINE_VALUE_SIGNAL(51, button_press, string);
    DEFINE_VALUE_SIGNAL(52, visibility_change, bool);
    DEFINE_VALUE_SIGNAL(53, dim_window, double);
    DEFINE_SIGNAL(54, shade_window);
    DEFINE_SIGNAL(55, unshade_window);
  }

  namespace parser {
    using parser_t = polybar::parser;

    DEFINE_VALUE_SIGNAL(70, change_background, uint32_t);
    DEFINE_VALUE_SIGNAL(71, change_foreground, uint32_t);
    DEFINE_VALUE_SIGNAL(72, change_underline, uint32_t);
    DEFINE_VALUE_SIGNAL(73, change_overline, uint32_t);
    DEFINE_VALUE_SIGNAL(74, change_font, uint8_t);
    DEFINE_VALUE_SIGNAL(75, change_alignment, alignment);
    DEFINE_VALUE_SIGNAL(76, offset_pixel, int16_t);
    DEFINE_VALUE_SIGNAL(77, attribute_set, attribute);
    DEFINE_VALUE_SIGNAL(78, attribute_unset, attribute);
    DEFINE_VALUE_SIGNAL(79, attribute_toggle, attribute);
    DEFINE_VALUE_SIGNAL(80, action_begin, action);
    DEFINE_VALUE_SIGNAL(81, action_end, mousebtn);
    DEFINE_VALUE_SIGNAL(82, write_text_ascii, uint16_t);
    DEFINE_VALUE_SIGNAL(83, write_text_unicode, uint16_t);
    DEFINE_VALUE_SIGNAL(84, write_text_string, parser_t::packet);
  }
}

POLYBAR_NS_END
