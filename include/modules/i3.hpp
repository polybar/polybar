#pragma once

#include <i3ipc++/ipc.hpp>

#include "components/config.hpp"
#include "modules/meta/event_module.hpp"
#include "modules/meta/input_handler.hpp"
#include "utils/i3.hpp"
#include "utils/io.hpp"

POLYBAR_NS

namespace modules {
  class i3_module : public event_module<i3_module>, public input_handler {
   public:
    enum class state {
      NONE,
      /**
       * \brief Active workspace on focused monitor
       */
      FOCUSED,
      /**
       * \brief Inactive workspace on any monitor
       */
      UNFOCUSED,
      /**
       * \brief Active workspace on unfocused monitor
       */
      VISIBLE,
      /**
       * \brief Workspace with urgency hint set
       */
      URGENT,
    };

    struct workspace {
      explicit workspace(string name, enum state state_, label_t&& label)
          : name(name), state(state_), label(forward<label_t>(label)) {}

      operator bool();

      string name;
      enum state state;
      label_t label;
    };

   public:
    explicit i3_module(const bar_settings&, string);

    void stop();
    bool has_event();
    bool update();
    bool build(builder* builder, const string& tag) const;

   protected:
    bool input(string&& cmd);

   private:
    static string make_workspace_command(const string& workspace);

    static constexpr const char* DEFAULT_TAGS{"<label-state> <label-mode>"};
    static constexpr const char* DEFAULT_MODE{"default"};
    static constexpr const char* DEFAULT_WS_ICON{"ws-icon-default"};
    static constexpr const char* DEFAULT_WS_LABEL{"%icon% %name%"};

    static constexpr const char* TAG_LABEL_STATE{"<label-state>"};
    static constexpr const char* TAG_LABEL_MODE{"<label-mode>"};

    static constexpr const char* EVENT_PREFIX{"i3wm"};
    static constexpr const char* EVENT_CLICK{"i3wm-wsfocus-"};
    static constexpr const char* EVENT_SCROLL_UP{"i3wm-wsnext"};
    static constexpr const char* EVENT_SCROLL_DOWN{"i3wm-wsprev"};

    map<state, label_t> m_statelabels;
    vector<unique_ptr<workspace>> m_workspaces;
    iconset_t m_icons;

    label_t m_modelabel;
    bool m_modeactive{false};

    /**
     * Separator that is inserted in between workspaces
     */
    label_t m_labelseparator;

    bool m_click{true};
    bool m_scroll{true};
    bool m_revscroll{true};
    bool m_wrap{true};
    bool m_indexsort{false};
    bool m_pinworkspaces{false};
    bool m_strip_wsnumbers{false};
    bool m_fuzzy_match{false};

    unique_ptr<i3_util::connection_t> m_ipc;
  };
}  // namespace modules

POLYBAR_NS_END
