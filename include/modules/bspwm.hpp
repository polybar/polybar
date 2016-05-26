#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "modules/base.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  namespace Bspwm
  {
    enum Flag
    {
      WORKSPACE_NONE,
      WORKSPACE_ACTIVE,
      WORKSPACE_URGENT,
      WORKSPACE_EMPTY,
      WORKSPACE_OCCUPIED,
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

    std::map<Bspwm::Flag, std::unique_ptr<drawtypes::Label>> mode_labels;
    std::map<Bspwm::Flag, std::unique_ptr<drawtypes::Label>> state_labels;

    std::vector<std::unique_ptr<Bspwm::Workspace>> workspaces;
    std::vector<std::unique_ptr<drawtypes::Label>*> modes;

    std::unique_ptr<drawtypes::IconMap> icons;
    std::string monitor;

    int socket_fd = -1;
    std::string prev_data;

    public:
      BspwmModule(const std::string& name, const std::string& monitor);
      ~BspwmModule() { close(this->socket_fd); }

      void start();
      bool has_event();
      bool update();
      bool build(Builder *builder, const std::string& tag);
      bool handle_command(const std::string& cmd);
  };
}
