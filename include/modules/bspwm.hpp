#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "modules/base.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  namespace bspwm
  {
    typedef struct payload_t {
      char data[BUFSIZ];
      size_t len = 0;
    } payload_t;

    enum Flag
    {
      WORKSPACE_NONE,
      WORKSPACE_URGENT,
      WORKSPACE_EMPTY,
      WORKSPACE_OCCUPIED,

      WORKSPACE_FOCUSED_URGENT,
      WORKSPACE_FOCUSED_EMPTY,
      WORKSPACE_FOCUSED_OCCUPIED,

      // used when the monitor is unfocused
      WORKSPACE_DIMMED,

      MODE_NONE,
      MODE_LAYOUT_MONOCLE,
      MODE_LAYOUT_TILED,
      MODE_STATE_FULLSCREEN,
      MODE_STATE_FLOATING,
      MODE_NODE_LOCKED,
      MODE_NODE_STICKY,
      MODE_NODE_PRIVATE
    };

    struct Workspace
    {
      Flag flag;
      std::unique_ptr<drawtypes::Label> label;

      Workspace(Flag flag, std::unique_ptr<drawtypes::Label> label) {
        this->flag = flag;
        this->label.swap(label);
      }

      operator bool() { return this->label && *this->label; }
    };
  }

  DefineModule(BspwmModule, EventModule)
  {
    static constexpr auto TAG_LABEL_STATE = "<label-state>";
    static constexpr auto TAG_LABEL_MODE = "<label-mode>";

    static constexpr auto EVENT_CLICK = "bwm";

    std::map<bspwm::Flag, std::unique_ptr<drawtypes::Label>> mode_labels;
    std::map<bspwm::Flag, std::unique_ptr<drawtypes::Label>> state_labels;

    std::vector<std::unique_ptr<bspwm::Workspace>> workspaces;
    std::vector<std::unique_ptr<drawtypes::Label>*> modes;

    std::unique_ptr<drawtypes::IconMap> icons;
    std::string monitor;

    int socket_fd = -1;
    std::string prev_data;

    public:
      BspwmModule(std::string name, std::string monitor);
      ~BspwmModule();

      void start();
      bool has_event();
      bool update();
      bool build(Builder *builder, std::string tag);

      bool handle_command(std::string cmd);
      bool register_for_events() const {
        return true;
      }
  };
}
