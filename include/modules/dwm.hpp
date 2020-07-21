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
    using window_t = unsigned int;

    /**
     * Represents the relevant states a tag can have
     */
    enum class state_t : uint8_t {
      FOCUSED,    ///< Monitor is selected and tag is selected, overrides all below
      URGENT,     ///< Tag is urgent, overrides all below
      UNFOCUSED,  ///< Monitor is not selected, but tag is selected
      VISIBLE,    ///< Tag is not selected, but occupied
      EMPTY       ///< Tag is unoccupied and unselected
    };

    /**
     * Associates important properties of a tag
     */
    struct tag_t {
      /**
       * Construct a tag_t object
       *
       * @param name Name of tag
       * @param tag_mask_t Bit mask that represents this tag
       * @param state Current state of the tag
       * @param label Label to use for building tag on bar
       */
      tag_t(string& name, tag_mask_t bit_mask, state_t state, label_t&& label)
          : name(name), bit_mask(bit_mask), state(state), label(forward<label_t>(label)) {}

      string name;
      tag_mask_t bit_mask;
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
    static constexpr const char* DEFAULT_STATE_LABEL{"%name%"};

    /**
     * The state label is used to represent a tag. This label is replaced by one
     * of the following labels based on the tag state:
     *   * label-focused
     *   * label-unfocused
     *   * label-visible
     *   * label-urgent
     *   * label-empty
     */
    static constexpr const char* TAG_LABEL_STATE{"<label-state>"};

    /**
     * The layout label is replaced by the current layout symbol
     */
    static constexpr const char* TAG_LABEL_LAYOUT{"<label-layout>"};

    /**
     * The title layout is replaced by the currently focused window title
     */
    static constexpr const char* TAG_LABEL_TITLE{"<label-title>"};

    static constexpr const char* EVENT_PREFIX{"dwm-"};

    /**
     * Event name is same as the IPC command name to view the tag clicked on
     */
    static constexpr const char* EVENT_TAG_LCLICK{"view"};

    /**
     * Event name is same as IPC command name to toggle the view of the tag
     * clicked on
     */
    static constexpr const char* EVENT_TAG_RCLICK{"toggleview"};

    /**
     * Called by has_event on layout changes. This updates the layout label
     *
     * @param ev Event data
     */
    void on_layout_change(const dwmipc::LayoutChangeEvent& ev);

    /**
     * Called by has_event when a new monitor is in focus. This updates
     * m_active_mon to keep track of the currently active monitor.
     *
     * @param ev Event data
     */
    void on_monitor_focus_change(const dwmipc::MonitorFocusChangeEvent& ev);

    /**
     * Called by has_event when any of the tag states change. This updates the
     * m_tags array and their states/labels.
     *
     * @param ev Event data
     */
    void on_tag_change(const dwmipc::TagChangeEvent& ev);

    /**
     * Called by has_event when a new client is in focus. This updates
     * m_focused_client_id and updates the title label
     *
     * @param ev Event data
     */
    void on_client_focus_change(const dwmipc::ClientFocusChangeEvent& ev);

    /**
     * Called by has_event when the title of the currently focused window
     * changes. This updates the title label.
     *
     * @param ev Event data
     */
    void on_focused_title_change(const dwmipc::FocusedTitleChangeEvent& ev);

    /**
     * Get a list of monitors from dwm, store them in m_monitors, and update the
     * pointers to the active monitor and bar monitor.
     *
     * @param ev Event data
     */
    void update_monitor_ref();

    /**
     * Update the labels for each tag based on their state
     */
    void update_tag_labels();

    /**
     * Get the window title of the specified client from dwm and update the
     * title label
     *
     * @param client_id The window XID of the client
     */
    void update_title_label(window_t client_id);

    /**
     * Translate the tag's tag states to a state_t enum value
     *
     * @param bit_mask Bit mask of the tag
     *
     * @return state_t enum value representing the state of the tag
     */
    auto get_state(tag_mask_t bit_mask) const -> state_t;

    /**
     * Check if the command matches the specified event name and if so, send a
     * command to dwm after parsing the command.
     *
     * @param cmd The command string given by dwm_modue::input
     * @param ev_name The name of the event to check for (should be the same as
     *   the dwm command name)
     *
     * @return true if the command matched, was succesfully parsed, and sent to
     *   dwm, false otherwise
     */
    auto check_send_cmd(string cmd, const string& ev_name) -> bool;

    /**
     * Helper function to build cmd string
     *
     * @param ev The event name (should be same as dwm command name)
     * @param arg The argument to the dwm command
     */
    auto static build_cmd(const char* ev, const string& arg) -> string;

    /**
     * Attempt to connect to any disconnected dwm sockets. Catch errors.
     */
    auto reconnect_dwm() -> bool;

    /**
     * If true, enables the click handlers for the tags
     */
    bool m_click{true};

    /**
     * Holds the address to the currently active monitor in the m_monitors array
     */
    const dwmipc::Monitor* m_active_mon = nullptr;

    /**
     * Holds the address to the bar monitor in the m_monitors array
     */
    const dwmipc::Monitor* m_bar_mon = nullptr;

    /**
     * XID of the currently focused client
     */
    window_t m_focused_client_id = 0;

    /**
     * Current layout symbol
     */
    label_t m_layout_label;

    /**
     * Inserted between tags
     */
    label_t m_seperator_label;

    /**
     * Title of the currently focused window on the bar's monitor
     */
    label_t m_title_label;

    /**
     * Connection to DWM
     */
    unique_ptr<dwmipc::Connection> m_ipc;

    /**
     * Vector of monitors returned by m_ipc->get_monitors
     */
    shared_ptr<vector<dwmipc::Monitor>> m_monitors;

    /**
     * Maps state_t enum values to their corresponding labels
     */
    std::unordered_map<state_t, label_t> m_state_labels;

    /**
     * Vector of all tags
     */
    vector<tag_t> m_tags;
  };
}  // namespace modules

POLYBAR_NS_END
