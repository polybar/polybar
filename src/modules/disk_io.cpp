#include <fstream>
#include <iomanip>
#include <istream>
#include <vector>
#include <regex>
#include <sys/types.h>
#include <dirent.h>

#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "modules/disk_io.hpp"
#include "utils/math.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<disk_io_module>;

  disk_io_module::disk_io_module(const bar_settings& bar, string name_) : timer_module<disk_io_module>(bar, move(name_)) {
    m_interval = m_conf.get<decltype(m_interval)>(name(), "interval", 1s);

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL}/*, TAG_BAR_READ, TAG_BAR_WRITE}*/);
    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%speed_read% Mb/s %speed_write% Mb/s");
    }
    m_disk_names = _get_disk_names();
    for (auto name : m_disk_names) {
      std::cout << "name: " << name << std::endl;
    }
}

std::vector<std::string> disk_io_module::_get_disk_names(void) {
  DIR *dp;
  struct dirent *dirp;
  std::vector<std::string> names;

  if (!(dp = opendir("/sys/block/")))
  {
    std::cout << "Can't open /sys/block!!" << std::endl;
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

std::pair<unsigned long long, unsigned long long> disk_io_module::_get_disk_read_write(std::string disk_name) {
  unsigned long long sec_read_total_new{0ULL};
  unsigned long long sec_write_total_new{0ULL};
  try {
    std::ifstream disk_io_info("/sys/block/" + disk_name + "/stat");

    std::string line;
    std::getline(disk_io_info, line);

    static const std::regex re{"\\s+"};
    std::vector<std::string> items{
      std::sregex_token_iterator(line.begin(), line.end(), re, -1),
        std::sregex_token_iterator()
    };

    std::vector<std::string> filtered_items;

    for (const auto &item : items) {
      if (item.length() > 0)
        filtered_items.push_back(item);
    }

    sec_read_total_new = std::strtoull(filtered_items[READ_SECTORS_OFFSET].c_str(), nullptr, 10);
    sec_write_total_new = std::strtoull(filtered_items[WRITE_SECTORS_OFFSET].c_str(), nullptr, 10);
  } catch (const std::exception& err) {
    m_log.err("Failed to read memory values (what: %s)", err.what());
  }
  return (std::make_pair(sec_read_total_new, sec_write_total_new));
}

float disk_io_module::_get_time_delta(void) {
  std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());

  std::chrono::duration<float> ms_delta = now - m_ms;
  float time_delta = ms_delta.count();
  m_ms = now;
  return time_delta;
}

void disk_io_module::_calculate_disk_io_speeds(std::string disk_name)
{
  unsigned long long read_total_new{0ULL};
  unsigned long long write_total_new{0ULL};

  auto read_write_pair = _get_disk_read_write(disk_name);
  read_total_new = read_write_pair.first;
  write_total_new = read_write_pair.second;
  if (read_total_new == 0ULL || write_total_new == 0ULL)
    return ;

  /*
   * /sys/block/name/stat third and seventh fields display total compleated
   * reads / writes in standard UNIX blocks (512b) so we need to divide delta
   * by 2048 to get value in Mb
   */
  m_read_speeds[disk_name] = static_cast<float>(read_total_new - m_read_total[disk_name])
    / (2.0f * 1024.0f * m_time_delta);
  m_write_speeds[disk_name] = static_cast<float>(write_total_new - m_write_total[disk_name])
    / (2.0f * 1024.0f * m_time_delta);

  // replace old values
  m_read_total[disk_name] = read_total_new;
  m_write_total[disk_name] = write_total_new;
}

bool disk_io_module::update() {
  m_time_delta = _get_time_delta();

  float sum_read{0.0f};
  float sum_write{0.0f};

  for (auto disk_name : m_disk_names) {
    _calculate_disk_io_speeds(disk_name);
    sum_read += m_read_speeds[disk_name];
    sum_write += m_write_speeds[disk_name];
  }

  // replace tokens
  if (m_label) {
    std::ostringstream ss;
    m_label->reset_tokens();
    ss << std::fixed << std::setprecision(2) << (sum_read);
    m_label->replace_token("%speed_read%", ss.str());
    ss.str("");
    ss << std::setprecision(2) << (sum_write);
    m_label->replace_token("%speed_write%", ss.str());
    ss.str("");
  }

  return true;
}

bool disk_io_module::build(builder* builder, const string& tag) const {
  if (tag == TAG_LABEL) {
    builder->node(m_label);
  } else {
    return false;
  }

  return true;
}
}

POLYBAR_NS_END
