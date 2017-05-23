#pragma once

#include "common.hpp"

#include "components/ipc.hpp"
#include "components/parser.hpp"
#include "components/types.hpp"
#include "utils/functional.hpp"

POLYBAR_NS

// fwd
enum class mousebtn;
enum class syntaxtag;
enum class alignment;
enum class attribute;

namespace signals {
  namespace detail {
    class signal {
     public:
      explicit signal() = default;
      virtual ~signal() {}
      virtual size_t size() const = 0;
    };

    template <typename Derived>
    class base_signal : public signal {
     public:
      using base_type = base_signal<Derived>;

      explicit base_signal() = default;
      virtual ~base_signal() {}

      virtual size_t size() const override {
        return sizeof(Derived);
      };
    };

    template <typename Derived, typename ValueType>
    class value_signal : public base_signal<Derived> {
     public:
      using base_type = value_signal<Derived, ValueType>;

      explicit value_signal(void* data) : m_ptr(data) {}
      explicit value_signal(ValueType&& data) : m_ptr(&data) {}

      virtual ~value_signal() {}

      inline ValueType cast() const {
        return *static_cast<ValueType*>(m_ptr);
      }

     private:
      void* m_ptr;
    };
  }

  namespace eventqueue {
    struct start : public detail::base_signal<start> {
      using base_type::base_type;
    };
    struct exit_terminate : public detail::base_signal<exit_terminate> {
      using base_type::base_type;
    };
    struct exit_reload : public detail::base_signal<exit_reload> {
      using base_type::base_type;
    };
    struct notify_change : public detail::base_signal<notify_change> {
      using base_type::base_type;
    };
    struct notify_forcechange : public detail::base_signal<notify_forcechange> {
      using base_type::base_type;
    };
    struct check_state : public detail::base_signal<check_state> {
      using base_type::base_type;
    };
  }

  namespace ipc {
    struct command : public detail::value_signal<command, string> {
      using base_type::base_type;
    };
    struct hook : public detail::value_signal<hook, string> {
      using base_type::base_type;
    };
    struct action : public detail::value_signal<action, string> {
      using base_type::base_type;
    };
  }

  namespace ui {
    struct ready : public detail::base_signal<ready> {
      using base_type::base_type;
    };
    struct changed : public detail::base_signal<changed> {
      using base_type::base_type;
    };
    struct tick : public detail::base_signal<tick> {
      using base_type::base_type;
    };
    struct button_press : public detail::value_signal<button_press, string> {
      using base_type::base_type;
    };
    struct cursor_change : public detail::value_signal<cursor_change, string> {
      using base_type::base_type;
    };
    struct visibility_change : public detail::value_signal<visibility_change, bool> {
      using base_type::base_type;
    };
    struct dim_window : public detail::value_signal<dim_window, double> {
      using base_type::base_type;
    };
    struct shade_window : public detail::base_signal<shade_window> {
      using base_type::base_type;
    };
    struct unshade_window : public detail::base_signal<unshade_window> {
      using base_type::base_type;
    };
    struct request_snapshot : public detail::value_signal<request_snapshot, string> {
      using base_type::base_type;
    };
    struct update_background : public detail::base_signal<update_background> {
      using base_type::base_type;
    };
  }

  namespace ui_tray {
    struct mapped_clients : public detail::value_signal<mapped_clients, unsigned int> {
      using base_type::base_type;
    };
  }

  namespace parser {
    struct change_background : public detail::value_signal<change_background, unsigned int> {
      using base_type::base_type;
    };
    struct change_foreground : public detail::value_signal<change_foreground, unsigned int> {
      using base_type::base_type;
    };
    struct change_underline : public detail::value_signal<change_underline, unsigned int> {
      using base_type::base_type;
    };
    struct change_overline : public detail::value_signal<change_overline, unsigned int> {
      using base_type::base_type;
    };
    struct change_font : public detail::value_signal<change_font, int> {
      using base_type::base_type;
    };
    struct change_alignment : public detail::value_signal<change_alignment, alignment> {
      using base_type::base_type;
    };
    struct reverse_colors : public detail::base_signal<reverse_colors> {
      using base_type::base_type;
    };
    struct offset_pixel : public detail::value_signal<offset_pixel, int> {
      using base_type::base_type;
    };
    struct attribute_set : public detail::value_signal<attribute_set, attribute> {
      using base_type::base_type;
    };
    struct attribute_unset : public detail::value_signal<attribute_unset, attribute> {
      using base_type::base_type;
    };
    struct attribute_toggle : public detail::value_signal<attribute_toggle, attribute> {
      using base_type::base_type;
    };
    struct action_begin : public detail::value_signal<action_begin, action> {
      using base_type::base_type;
    };
    struct action_end : public detail::value_signal<action_end, mousebtn> {
      using base_type::base_type;
    };
    struct text : public detail::value_signal<text, string> {
      using base_type::base_type;
    };
  }
}

POLYBAR_NS_END
