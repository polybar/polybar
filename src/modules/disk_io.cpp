#include <fstream>
#include <iomanip>
#include <istream>
#include <vector>
#include <regex>
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>

#include "drawtypes/label.hpp"
#include "drawtypes/iconset.hpp"
#include "modules/disk_io.hpp"
#include "utils/math.hpp"

#include "modules/meta/base.inl"

#include <iostream>
POLYBAR_NS

namespace modules {
  template class module<disk_io_module>;

  disk_io_module::disk_io_module(const bar_settings& bar, string name_) : timer_module<disk_io_module>(bar, move(name_)) {
    std::vector<std::string> default_disk_name;

    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 1s);
    m_disk_names = m_conf.get_list(name(), "monitored-disks", default_disk_name); 

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_ICON_READ, TAG_ICON_WRITE});
    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "R: %speed_read% W: %speed_write%");
    }

    m_icons = factory_util::shared<iconset>();

    if (m_formatter->has(TAG_ICON_READ)) {
      m_icons->add("read", load_icon(m_conf, name(), TAG_ICON_READ));
    }
    if (m_formatter->has(TAG_ICON_WRITE)) {
      m_icons->add("write", load_icon(m_conf, name(), TAG_ICON_WRITE));
    }
    if (m_formatter->has(TAG_ICON_READ) || m_formatter->has(TAG_ICON_WRITE)) {
      m_toggle_on_color = m_conf.get(name(), "toggle-on-foreground", ""s);
      m_toggle_off_color = m_conf.get(name(), "toggle-off-foreground", ""s);
    }
    if (m_disk_names.size() == 0) {
      m_disk_names = get_disk_names();
    }
}

std::vector<std::string> disk_io_module::get_disk_names(void) {
  DIR *dp;
  struct dirent *dirp;
  std::vector<std::string> names;

  if (!(dp = opendir("/sys/block/")))
  {
    m_log.err("Can't open /sys/block!!");
    return names;
  }
  while ((dirp = readdir(dp))) {
    if (dirp->d_name[0] != '.') {
      names.push_back(std::string(dirp->d_name));
    }
  }
  closedir(dp);
  return names;
}

std::pair<unsigned long long, unsigned long long> disk_io_module::get_disk_read_write(std::string disk_name) {
  unsigned long long sec_read_total_new{0ULL};
  unsigned long long sec_write_total_new{0ULL};
  try {
    std::ifstream disk_io_info("/sys/block/" + disk_name + "/stat");

    std::string line;
    std::getline(disk_io_info, line);

    std::vector<std::string> items = string_util::split(line, ' ');
    std::vector<std::string> filtered_items;

    for (const auto &item : items) {
      if (item.length() > 0) {
        filtered_items.push_back(item);
      }
    }

    sec_read_total_new = std::strtoull(filtered_items[READ_SECTORS_OFFSET].c_str(), nullptr, 10);
    sec_write_total_new = std::strtoull(filtered_items[WRITE_SECTORS_OFFSET].c_str(), nullptr, 10);
  } catch (const std::exception& err) {
    m_log.err("Failed to read diskstat values (what: %s)", err.what());
  }
  return (std::make_pair(sec_read_total_new, sec_write_total_new));
}

float disk_io_module::get_time_delta(void) {
  std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());

  std::chrono::duration<float> ms_delta = now - m_ms;
  float time_delta = ms_delta.count();
  m_ms = now;
  return time_delta;
}

void disk_io_module::calculate_disk_io_speeds(std::string disk_name)
{
  unsigned long long read_total_new{0ULL};
  unsigned long long write_total_new{0ULL};

  auto read_write_pair = get_disk_read_write(disk_name);
  read_total_new = read_write_pair.first;
  write_total_new = read_write_pair.second;
  if (read_total_new == 0ULL || write_total_new == 0ULL)
    return ;

  /*
   * /sys/block/name/stat third and seventh fields display total completed
   * reads / writes in standard UNIX blocks (512b) so we need to divide delta
   * by 2048 to get value in Mb
   */
  m_read_speeds[disk_name] = static_cast<float>(read_total_new - m_read_total[disk_name])
    / (2.0f * m_time_delta);
  m_write_speeds[disk_name] = static_cast<float>(write_total_new - m_write_total[disk_name])
    / (2.0f * m_time_delta);

  // replace old values
  m_read_total[disk_name] = read_total_new;
  m_write_total[disk_name] = write_total_new;
}

bool disk_io_module::update() {
  m_time_delta = get_time_delta();

  float sum_read{0.0f};
  float sum_write{0.0f};

  for (auto disk_name : m_disk_names) {
    calculate_disk_io_speeds(disk_name);
    sum_read += m_read_speeds[disk_name];
    sum_write += m_write_speeds[disk_name];
  }

  m_max_read_speed = std::max(m_max_read_speed, sum_read);
  m_max_write_speed = std::max(m_max_write_speed, sum_write);

  // replace tokens
  if (m_label) {
    m_label->reset_tokens();
    m_label->replace_token("%speed_read%", string_util::filesize_mb(sum_read, 1, m_bar.locale) + "/s");
    m_label->replace_token("%speed_write%", string_util::filesize_mb(sum_write, 1, m_bar.locale) + "/s");
    if (m_icons->has("read")) {
      m_icons->get("read")->m_foreground = sum_read == 0 ? m_toggle_on_color : m_toggle_off_color;
    }
    if (m_icons->has("write")) {
      m_icons->get("write")->m_foreground = sum_write == 0 ? m_toggle_on_color : m_toggle_off_color;
    }
  }

  return true;
}

bool disk_io_module::build(builder* builder, const string& tag) const {
  if (tag == TAG_LABEL) {
    builder->node(m_label);
  } else if (tag == TAG_ICON_READ) {
    builder->node(m_icons->get("read"));
  } else if (tag == TAG_ICON_WRITE) {
    builder->node(m_icons->get("write"));
  } else {
    return false;
  }

  return true;
}
}

POLYBAR_NS_END
