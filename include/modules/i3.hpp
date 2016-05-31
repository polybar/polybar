#pragma once

#include <memory>
#include <unistd.h>
#include <i3ipc++/ipc.hpp>

#include "config.hpp"
#include "modules/base.hpp"
#include "drawtypes/icon.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  namespace i3
  {
    enum Flag
    {
      WORKSPACE_NONE,
      WORKSPACE_FOCUSED,
      WORKSPACE_UNFOCUSED,
      WORKSPACE_VISIBLE,
      WORKSPACE_URGENT,
      // used when the monitor is unfocused
      WORKSPACE_DIMMED,
    };

    struct Workspace
    {
      int idx;
      Flag flag;
      std::unique_ptr<drawtypes::Label> label;

      Workspace(int idx, Flag flag, std::unique_ptr<drawtypes::Label> label) {
        this->idx = idx;
        this->flag = flag;
        this->label.swap(label);
      }

      operator bool() { return this->label && *this->label; }
    };
  }

  DefineModule(i3Module, EventModule)
  {
    static constexpr auto TAG_LABEL_STATE = "<label:state>";

    static constexpr auto EVENT_CLICK = "i3";

    std::unique_ptr<i3ipc::connection> ipc;

    // std::map<i3::Flag, std::unique_ptr<drawtypes::Label>> mode_labels;
    std::map<i3::Flag, std::unique_ptr<drawtypes::Label>> state_labels;

    std::vector<std::unique_ptr<i3::Workspace>> workspaces;
    // std::vector<std::unique_ptr<drawtypes::Label>*> modes;

    std::unique_ptr<drawtypes::IconMap> icons;
    std::string monitor;

    bool local_workspaces = true;
    std::size_t workspace_name_strip_nchars = 0;

    int ipc_fd = -1;

    public:
      i3Module(const std::string& name, const std::string& monitor);

      void start();
      void stop();

      bool has_event();
      bool update();
      bool build(Builder *builder, const std::string& tag);

      bool handle_command(const std::string& cmd);
  };
}
