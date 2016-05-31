#pragma once

#include "modules/base.hpp"
#include "drawtypes/bar.hpp"
#include "drawtypes/label.hpp"

namespace modules
{
  struct Torrent
  {
    std::string title;
    unsigned long data_total;
    unsigned long data_downloaded;
    unsigned long data_remaining;
    float perc_downloaded;
    std::unique_ptr<drawtypes::Label> label_tokenized;
  };

  DefineModule(TorrentModule, InotifyModule)
  {
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR_PROGRESS = "<bar:progress>";

    std::vector<std::unique_ptr<Torrent>> torrents;
    std::unique_ptr<drawtypes::Label> label;
    std::unique_ptr<drawtypes::Label> label_tokenized;
    std::unique_ptr<drawtypes::Bar> bar;
    std::string pipe_cmd;

    std::vector<std::unique_ptr<Torrent>> &read_data_into(std::vector<std::unique_ptr<Torrent>> &container);

    public:
      explicit TorrentModule(const std::string& name);

      void start();
      bool on_event(InotifyEvent *event);
      bool build(Builder *builder, const std::string& tag);
  };
}
