#pragma once

#include "common.hpp"
#include "components/types.hpp"

POLYBAR_NS

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
      explicit value_signal(const ValueType&& data) : m_ptr(&data) {}
      explicit value_signal(const ValueType& data) : m_ptr(&data) {}

      virtual ~value_signal() {}

      inline const ValueType cast() const {
        return *static_cast<const ValueType*>(m_ptr);
      }

     private:
      const void* m_ptr;
    };
  } // namespace detail

  namespace eventqueue {
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
  } // namespace eventqueue

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
  } // namespace ipc

  namespace ui {
    struct changed : public detail::base_signal<changed> {
      using base_type::base_type;
    };
    struct button_press : public detail::value_signal<button_press, string> {
      using base_type::base_type;
    };
    struct visibility_change : public detail::value_signal<visibility_change, bool> {
      using base_type::base_type;
    };
    struct dim_window : public detail::value_signal<dim_window, double> {
      using base_type::base_type;
    };
    struct request_snapshot : public detail::value_signal<request_snapshot, string> {
      using base_type::base_type;
    };
    /// emitted whenever the desktop background slice changes
    struct update_background : public detail::base_signal<update_background> {
      using base_type::base_type;
    };
    /// emitted when the bar geometry changes (such as position of the bar on the screen)
    struct update_geometry : public detail::base_signal<update_geometry> {
      using base_type::base_type;
    };
  } // namespace ui

  namespace ui_tray {
    struct tray_pos_change : public detail::value_signal<tray_pos_change, int> {
      using base_type::base_type;
    };
  } // namespace ui_tray
} // namespace signals

POLYBAR_NS_END
