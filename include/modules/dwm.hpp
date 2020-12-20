#pragma once

#include <dwmipcpp/connection.hpp>

#include "modules/meta/event_module.hpp"

POLYBAR_NS

namespace modules {
  class dwm_module : public event_module<dwm_module> {
   public:
    explicit dwm_module(const bar_settings&, string);

    using tag_mask_t = unsigned int;
    using window_t = unsigned int;

    /**
     * Represents the relevant states a tag can have
     */
    enum class state_t : uint8_t {
      URGENT,     ///< Tag is urgent, overrides all below
      FOCUSED,    ///< Monitor is selected and tag is selected, overrides all below
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

    static constexpr auto TYPE = "internal/dwm";

    void stop() override;
    bool has_event();
    bool update();
    bool build(builder* builder, const string& tag) const;

    /**
     * DWM command for changing the view to a tag with the specified bit mask
     */
    static constexpr const char* EVENT_TAG_VIEW{"view"};

    /**
     * DWM command for toggling the selected state of a tag with the specified
     * bit mask
     */
    static constexpr const char* EVENT_TAG_TOGGLE_VIEW{"toggleview"};

    /**
     * DWM command for setting the layout to a layout specified by the address
     */
    static constexpr const char* EVENT_LAYOUT_SET{"setlayoutsafe"};

   protected:
    bool input(const string& action, const string& data) override;

   private:
    static constexpr const char* DEFAULT_FORMAT_TAGS{"<label-tags> <label-layout> <label-floating> <label-title>"};
    static constexpr const char* DEFAULT_STATE_LABEL{"%name%"};

    /**
     * The tags label is replaced with the tags. Each tag is displayed using one
     * of the following labels based on the tag state:
     *   * label-focused
     *   * label-unfocused
     *   * label-visible
     *   * label-urgent
     *   * label-empty
     */
    static constexpr const char* TAG_LABEL_TAGS{"<label-tags>"};

    /**
     * The layout label is replaced by the current layout symbol
     */
    static constexpr const char* TAG_LABEL_LAYOUT{"<label-layout>"};

    /**
     * The floating label is shown when the selected window is floating
     */
    static constexpr const char* TAG_LABEL_FLOATING{"<label-floating>"};

    /**
     * The title layout is replaced by the currently focused window title
     */
    static constexpr const char* TAG_LABEL_TITLE{"<label-title>"};

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
     * Called by has_event when the state of the currently focused window
     * changes. This updates the floating label.
     *
     * @param ev Event data
     */
    void on_focused_state_change(const dwmipc::FocusedStateChangeEvent& ev);

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
     * Update the title label with the specified title
     *
     * @param title The title to use to update the label
     */
    void update_title_label(const string& title);

    /**
     * Get the window title of the currently focused client from dwm and update
     * the title label
     */
    void update_title_label();

    /**
     * Query dwm to determine if the currently focused client and update
     * m_is_floating
     */
    void update_floating_label();

    /**
     * Update the layout label with the specified symbol
     *
     * @param symbol The symbol to use to update the label
     */
    void update_layout_label(const string& symbol);

    /**
     * Translate the tag's tag states to a state_t enum value
     *
     * @param bit_mask Bit mask of the tag
     *
     * @return state_t enum value representing the state of the tag
     */
    state_t get_state(tag_mask_t bit_mask) const;

    /**
     * Get the address to the layout represented by the symbol.
     */
    const dwmipc::Layout* find_layout(const string& sym) const;

    /**
     * Get the address to the layout represented by the address.
     */
    const dwmipc::Layout* find_layout(uintptr_t addr) const;

    /**
     * Get the address of the next layout in m_layouts.
     *
     * @param layout Address of the current layout
     * @param wrap True to wrap around the array, false to return the same
     *   layout if the next layout does not exist.
     */
    const dwmipc::Layout* next_layout(const dwmipc::Layout& layout, bool wrap) const;

    /**
     * Get the address of the previous layout in m_layouts.
     *
     * @param layout Address of the current layout
     * @param wrap True to wrap around the array, false to return the same
     *   layout if the next layout does not exist.
     */
    const dwmipc::Layout* prev_layout(const dwmipc::Layout& layout, bool wrap) const;

    /**
     * Get the address of the next tag in m_tags or return NULL if not applicable
     *
     * @param ignore_empty Ignore empty tags
     */
    const tag_t* next_scrollable_tag(bool ignore_empty) const;

    /**
     * Get the address of the prev tag in m_tags or return NULL if not applicable
     *
     * @param ignore_empty Ignore empty tags
     */
    const tag_t* prev_scrollable_tag(bool ignore_empty) const;

    /**
     * Attempt to connect to any disconnected dwm sockets. Catch errors.
     */
    bool reconnect_dwm();

    /**
     * If true, enables the click handlers for the tags
     */
    bool m_tags_click{true};

    /**
     * If true, enables the click handlers for the layout label
     */
    bool m_layout_click{true};

    /**
     * If true, scrolling the layout cycle through available layouts
     */
    bool m_layout_scroll{true};

    /**
     * If true, scrolling the bar cycles through the available tags
     */
    bool m_tags_scroll{false};

    /**
     * If true, scrolling the bar cycles through the available tags backwards
     */
    bool m_tags_scroll_reverse{false};

    /**
     * If true, wrap tag when scrolling
     */
    bool m_tags_scroll_wrap{false};

    /**
     * If true, scrolling will view all tags regardless if occupied
     */
    bool m_tags_scroll_empty{false};

    /**
     * If true, scrolling the layout will wrap around to the beginning
     */
    bool m_layout_wrap{true};

    /**
     * If true, scrolling the layout will cycle layouts in the reverse direction
     */
    bool m_layout_reverse{false};

    /**
     * If true, show floating label
     */
    bool m_is_floating{false};

    /**
     * If the layout symbol is clicked on, it will set the layout represented by
     * this symbol. The default is monocle mode [M].
     */
    string m_secondary_layout_symbol{"[M]"};

    /**
     * DWM socket path. Can be overriden by config.
     */
    string m_socket_path{"/tmp/dwm.sock"};

    /**
     * Holds the address to the secondary layout specified by the secondary
     * layout symbol
     */
    const dwmipc::Layout* m_secondary_layout = nullptr;

    /**
     * Holds the address to the current layout
     */
    const dwmipc::Layout* m_current_layout = nullptr;

    /**
     * Holds the address to the default layout
     */
    const dwmipc::Layout* m_default_layout = nullptr;

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
     * Shown when currently focused window is floating
     */
    label_t m_floating_label;

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
     * Vector of layouts returned by m_ipc->get_layouts
     */
    shared_ptr<vector<dwmipc::Layout>> m_layouts;

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
