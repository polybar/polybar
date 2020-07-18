#pragma once

#include <dwmipcpp/connection.hpp>

#include "modules/meta/event_module.hpp"
#include "modules/meta/input_handler.hpp"

POLYBAR_NS

namespace modules {
  class dwm_module : public event_module<dwm_module>, public input_handler {
   public:
    explicit dwm_module(const bar_settings&, string);

    using tag_mask_t = unsigned int;

    enum class state_t : uint8_t {
      FOCUSED,    ///< Monitor is selected and tag is selected, overrides all below
      URGENT,     ///< Tag is urgent, overrides all below
      UNFOCUSED,  ///< Monitor is not selected, but tag is selected
      VISIBLE,    ///< Tag is not selected, but occupied
      NONE        ///< Tag is unoccupied and unselected
    };

    struct tag_t {
      tag_t(string& name, unsigned int bit_mask, state_t state, label_t&& label)
          : name(name), bit_mask(bit_mask), state(state), label(forward<label_t>(label)) {}

      string name;
      unsigned int bit_mask;
      state_t state;
      label_t label;
    };

    auto stop() -> void override;
    auto has_event() -> bool;
    auto update() -> bool;
    auto build(builder* builder, const string& tag) const -> bool;

   protected:
    auto input(string&& cmd) -> bool override;

   private:
    static constexpr const char* DEFAULT_FORMAT_TAGS{"<label-state> <label-layout> <label-title>"};
    static constexpr const char* DEFAULT_TAG_LABEL{"%name%"};

    static constexpr const char* TAG_LABEL_STATE{"<label-state>"};
    static constexpr const char* TAG_LABEL_LAYOUT{"<label-layout>"};
    static constexpr const char* TAG_LABEL_TITLE{"<label-title>"};

    static constexpr const char* EVENT_PREFIX{"dwm"};
    static constexpr const char* EVENT_LCLICK{"dwm-view-"};
    static constexpr const char* EVENT_RCLICK{"dwm-toggleview-"};
    static constexpr const char* EVENT_SCROLL_UP{"dwm-tagnext"};
    static constexpr const char* EVENT_SCROLL_DOWN{"dwm-tagprev"};

    auto get_state(tag_mask_t bit_mask) const -> state_t;
    void update_monitor_ref();

    bool m_click{true};
    bool m_pin_tags{false};

    label_t m_layout_label;
    label_t m_seperator_label;
    label_t m_title_label;

    shared_ptr<std::vector<dwmipc::Monitor>> m_monitors;
    unsigned int m_active_mon_num = 0;
    unsigned int m_bar_mon = 0;
    unsigned int m_focused_client_id = 0;

    std::unordered_map<state_t, label_t> m_state_labels;
    vector<tag_t> m_tags;

    unique_ptr<dwmipc::Connection> m_ipc;
  };
}  // namespace modules

POLYBAR_NS_END
