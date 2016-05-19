#include <string>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <sstream>

#include "lemonbuddy.hpp"
#include "services/command.hpp"
#include "services/builder.hpp"
#include "utils/io.hpp"
#include "utils/proc.hpp"
#include "utils/string.hpp"
#include "utils/config.hpp"
#include "modules/torrent.hpp"

using namespace modules;

// TODO: Parse the torrent files internally instead of using the bash script

TorrentModule::TorrentModule(const std::string& name_) : InotifyModule(name_)
{
  this->formatter->add(DEFAULT_FORMAT, TAG_LABEL, { TAG_LABEL, TAG_BAR_PROGRESS });

  if (this->formatter->has(TAG_LABEL))
    this->label = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL), "%title% (%percentage%)");
  if (this->formatter->has(TAG_BAR_PROGRESS))
    this->bar = drawtypes::get_config_bar(name(), get_tag_name(TAG_BAR_PROGRESS));

  auto rtorrent_session_dir = config::get<std::string>(name(), "rtorrent_session_dir");

  this->watch(rtorrent_session_dir);

  this->pipe_cmd = config::get<std::string>(name(), "script")
    + " " + rtorrent_session_dir
    + " " + std::to_string(config::get<int>(name(), "display_count", 2))
    + " " + std::to_string(config::get<int>(name(), "title_maxlen", 30));
}

void TorrentModule::start()
{
  this->InotifyModule<TorrentModule>::start();

  std::thread manual_updater([&]{
    while (this->enabled()) {
      std::this_thread::sleep_for(5s);
      this->on_event(nullptr);
    }
  });
  this->threads.emplace_back(std::move(manual_updater));
}

bool TorrentModule::on_event(InotifyEvent *event)
{
  if (EXIT_SUCCESS != std::system("pgrep rtorrent >/dev/null")) {
    get_logger()->debug("[modules::Torrent] rtorrent is not running... ");
    return true;
  }

  if (event != nullptr) {
    log_trace(event->filename);
  }

  this->torrents.clear();
  this->read_data_into(this->torrents);

  return true;
}

bool TorrentModule::build(Builder *builder, const std::string& tag)
{
  if (tag != TAG_LABEL && tag != TAG_BAR_PROGRESS)
    return false;

  for (auto &torrent : this->torrents) {
    if (tag == TAG_LABEL)
      builder->node(torrent->label_tokenized);
    else if (tag == TAG_BAR_PROGRESS)
      builder->node(this->bar, torrent->perc_downloaded);
  }

  return true;
}

std::vector<std::unique_ptr<Torrent>> &TorrentModule::read_data_into(std::vector<std::unique_ptr<Torrent>> &container)
{
  try {
    std::string buf;

    auto command = std::make_unique<Command>("/usr/bin/env\nsh\n-c\n"+ this->pipe_cmd);

    command->exec(false);

    while (!(buf = io::readline(command->get_stdout(PIPE_READ))).empty()) {
      auto values = string::split(buf, ':');

      if (values.size() != 4) {
        log_error("Bad value received from torrent script:\n"+ buf);
        continue;
      }

      auto torrent = std::make_unique<Torrent>();
      torrent->title = values[0];
      torrent->data_total = std::strtoul(values[1].c_str(), 0, 10);
      torrent->data_downloaded = std::strtoul(values[2].c_str(), 0, 10);
      torrent->data_remaining = std::strtoul(values[3].c_str(), 0, 10);
      torrent->perc_downloaded = (float) torrent->data_downloaded / torrent->data_total  * 100.0 + 0.5f;

      torrent->label_tokenized = this->label->clone();
      torrent->label_tokenized->replace_token("%title%", torrent->title);
      torrent->label_tokenized->replace_token("%percentage%", std::to_string((int) torrent->perc_downloaded)+"%");

      container.emplace_back(std::move(torrent));
    }

    command->wait();
  } catch (CommandException &e) {
    log_error(e.what());
  } catch (proc::ExecFailure &e) {
    log_error(e.what());
  }

  return container;
}
