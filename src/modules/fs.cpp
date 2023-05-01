#include "modules/fs.hpp"

#include <sys/statvfs.h>

#include <fstream>
#include <utility>

#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta/base.inl"
#include "utils/math.hpp"
#include "utils/string.hpp"

POLYBAR_NS

// Columns in /proc/self/mountinfo
#define MOUNTINFO_DIR 4
#define MOUNTINFO_TYPE 7
#define MOUNTINFO_FSNAME 8

namespace modules {
  template class module<fs_module>;

  /**
   * Bootstrap the module by reading config values and
   * setting up required components
   */
  fs_module::fs_module(const bar_settings& bar, string name_, const config& config)
      : timer_module<fs_module>(bar, move(name_), config) {
    m_mountpoints = m_conf.get_list(name(), "mount", {});
    if (m_mountpoints.empty()) {
      m_log.info("%s: No mountpoints specified, using fallback \"/\"", name());
      m_mountpoints.emplace_back("/");
    }
    m_remove_unmounted = m_conf.get(name(), "remove-unmounted", m_remove_unmounted);
    m_perc_used_warn = m_conf.get(name(), "warn-percentage", 90);
    m_fixed = m_conf.get(name(), "fixed-values", m_fixed);
    m_spacing = m_conf.get(name(), "spacing", m_spacing);
    set_interval(30s);

    // Add formats and elements
    m_formatter->add(
        FORMAT_MOUNTED, TAG_LABEL_MOUNTED, {TAG_LABEL_MOUNTED, TAG_BAR_FREE, TAG_BAR_USED, TAG_RAMP_CAPACITY});
    m_formatter->add_optional(FORMAT_WARN, {TAG_LABEL_WARN, TAG_BAR_FREE, TAG_BAR_USED, TAG_RAMP_CAPACITY});
    m_formatter->add(FORMAT_UNMOUNTED, TAG_LABEL_UNMOUNTED, {TAG_LABEL_UNMOUNTED});

    if (m_formatter->has(TAG_LABEL_MOUNTED)) {
      m_labelmounted = load_optional_label(m_conf, name(), TAG_LABEL_MOUNTED, "%mountpoint% %percentage_free%%");
    }
    if (m_formatter->has(TAG_LABEL_WARN)) {
      m_labelwarn = load_optional_label(m_conf, name(), TAG_LABEL_WARN, "%mountpoint% %percentage_free%%");
    }
    if (m_formatter->has(TAG_LABEL_UNMOUNTED)) {
      m_labelunmounted = load_optional_label(m_conf, name(), TAG_LABEL_UNMOUNTED, "%mountpoint% is not mounted");
    }
    if (m_formatter->has(TAG_BAR_FREE)) {
      m_barfree = load_progressbar(m_bar, m_conf, name(), TAG_BAR_FREE);
    }
    if (m_formatter->has(TAG_BAR_USED)) {
      m_barused = load_progressbar(m_bar, m_conf, name(), TAG_BAR_USED);
    }
    if (m_formatter->has(TAG_RAMP_CAPACITY)) {
      m_rampcapacity = load_ramp(m_conf, name(), TAG_RAMP_CAPACITY);
    }

    // Warn about "unreachable" format tag
    if (m_formatter->has(TAG_LABEL_UNMOUNTED) && m_remove_unmounted) {
      m_log.warn("%s: Defined format tag \"%s\" will never be used (reason: `remove-unmounted = true`)", name(),
          string{TAG_LABEL_UNMOUNTED});
    }
  }

  /**
   * Update mountpoints
   */
  bool fs_module::update() {
    m_mounts.clear();

    vector<vector<string>> mountinfo;
    std::ifstream filestream("/proc/self/mountinfo");
    string line;

    // Get details for mounted filesystems
    while (std::getline(filestream, line)) {
      auto cols = string_util::split(line, ' ');
      if (std::find(m_mountpoints.begin(), m_mountpoints.end(), cols[MOUNTINFO_DIR]) != m_mountpoints.end()) {
        mountinfo.emplace_back(move(cols));
      }
    }

    // Get data for defined mountpoints
    for (auto&& mountpoint : m_mountpoints) {
      auto details = std::find_if(mountinfo.begin(), mountinfo.end(),
          [&](const vector<string>& m) { return m.size() > 4 && m[MOUNTINFO_DIR] == mountpoint; });

      m_mounts.emplace_back(std::make_unique<fs_mount>(mountpoint, details != mountinfo.end()));
      struct statvfs buffer {};

      if (!m_mounts.back()->mounted) {
        m_log.warn("%s: Mountpoint %s is not mounted", name(), mountpoint);
      } else if (statvfs(mountpoint.c_str(), &buffer) == -1) {
        m_log.err("%s: Failed to query filesystem (statvfs() error: %s)", name(), strerror(errno));
      } else {
        auto& mount = m_mounts.back();
        mount->mountpoint = details->at(MOUNTINFO_DIR);
        mount->type = details->at(MOUNTINFO_TYPE);
        mount->fsname = details->at(MOUNTINFO_FSNAME);

        // see: https://en.cppreference.com/w/cpp/filesystem/space
        mount->bytes_total = static_cast<uint64_t>(buffer.f_frsize) * static_cast<uint64_t>(buffer.f_blocks);
        mount->bytes_free = static_cast<uint64_t>(buffer.f_frsize) * static_cast<uint64_t>(buffer.f_bfree);
        mount->bytes_used = mount->bytes_total - mount->bytes_free;
        mount->bytes_avail = static_cast<uint64_t>(buffer.f_frsize) * static_cast<uint64_t>(buffer.f_bavail);

        mount->percentage_free =
            math_util::percentage<double>(mount->bytes_avail, mount->bytes_used + mount->bytes_avail);
        mount->percentage_used =
            math_util::percentage<double>(mount->bytes_used, mount->bytes_used + mount->bytes_avail);
      }
    }

    if (m_remove_unmounted) {
      auto new_end =
          std::stable_partition(m_mounts.begin(), m_mounts.end(), [](const auto& mount) { return mount->mounted; });

      for (auto it = new_end; it < m_mounts.end(); ++it) {
        m_log.info("%s: Removing mountpoint \"%s\" (reason: `remove-unmounted = true`)", name(), (*it)->mountpoint);
        m_mountpoints.erase(
            std::remove(m_mountpoints.begin(), m_mountpoints.end(), (*it)->mountpoint), m_mountpoints.end());
      }

      m_mounts.erase(new_end, m_mounts.end());
    }

    return true;
  }

  /**
   * Generate the module output
   */
  string fs_module::get_output() {
    string output;

    for (m_index = 0_z; m_index < m_mounts.size(); ++m_index) {
      string mount_output = timer_module::get_output();
      /*
       * Add spacing before the mountpoint, but only if the mountpoint contents
       * are not empty and there is already other content in the module.
       */
      if (!output.empty() && !mount_output.empty()) {
        m_builder->spacing(m_spacing);
        output += m_builder->flush();
      }
      output += mount_output;
    }

    return output;
  }

  /**
   * Select format based on fs state
   */
  string fs_module::get_format() const {
    if (!m_mounts[m_index]->mounted) {
      return FORMAT_UNMOUNTED;
    }
    if (m_mounts[m_index]->percentage_used >= m_perc_used_warn && m_formatter->has_format(FORMAT_WARN)) {
      return FORMAT_WARN;
    }
    return FORMAT_MOUNTED;
  }

  /**
   * Output content using configured format tags
   */
  bool fs_module::build(builder* builder, const string& tag) const {
    auto& mount = m_mounts[m_index];

    auto replace_tokens = [&](const label_t& label) {
      label->reset_tokens();
      label->replace_token("%mountpoint%", mount->mountpoint);
      label->replace_token("%type%", mount->type);
      label->replace_token("%fsname%", mount->fsname);
      label->replace_token("%percentage_free%", to_string(mount->percentage_free));
      label->replace_token("%percentage_used%", to_string(mount->percentage_used));
      label->replace_token(
          "%total%", string_util::filesize(mount->bytes_total, m_fixed ? 2 : 0, m_fixed, m_bar.locale));
      label->replace_token("%free%", string_util::filesize(mount->bytes_avail, m_fixed ? 2 : 0, m_fixed, m_bar.locale));
      label->replace_token("%used%", string_util::filesize(mount->bytes_used, m_fixed ? 2 : 0, m_fixed, m_bar.locale));
    };

    if (tag == TAG_BAR_FREE) {
      builder->node(m_barfree->output(mount->percentage_free));
    } else if (tag == TAG_BAR_USED) {
      builder->node(m_barused->output(mount->percentage_used));
    } else if (tag == TAG_RAMP_CAPACITY) {
      builder->node(m_rampcapacity->get_by_percentage_with_borders(mount->percentage_free, 0, m_perc_used_warn));
    } else if (tag == TAG_LABEL_MOUNTED) {
      replace_tokens(m_labelmounted);
      builder->node(m_labelmounted);
    } else if (tag == TAG_LABEL_WARN) {
      replace_tokens(m_labelwarn);
      builder->node(m_labelwarn);
    } else if (tag == TAG_LABEL_UNMOUNTED) {
      m_labelunmounted->reset_tokens();
      m_labelunmounted->replace_token("%mountpoint%", mount->mountpoint);
      builder->node(m_labelunmounted);
    } else {
      return false;
    }

    return true;
  }
} // namespace modules

POLYBAR_NS_END
