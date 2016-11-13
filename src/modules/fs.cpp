#include <sys/statvfs.h>

#include "modules/fs.hpp"
#include "utils/math.hpp"
#include "utils/mtab.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

namespace modules {
  /**
   * Bootstrap the module by reading config values and
   * setting up required components
   */
  void fs_module::setup() {
    m_mounts = m_conf.get_list<string>(name(), "disk");
    m_fixed = m_conf.get<bool>(name(), "fixed-values", m_fixed);
    m_spacing = m_conf.get<int>(name(), "spacing", m_spacing);
    m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 30));

    // Add formats and elements
    m_formatter->add(
        FORMAT_MOUNTED, TAG_LABEL_MOUNTED, {TAG_LABEL_MOUNTED, TAG_BAR_FREE, TAG_BAR_USED, TAG_RAMP_CAPACITY});
    m_formatter->add(
        FORMAT_UNMOUNTED, TAG_LABEL_UNMOUNTED, {TAG_LABEL_UNMOUNTED, TAG_BAR_FREE, TAG_BAR_USED, TAG_RAMP_CAPACITY});

    if (m_formatter->has(TAG_LABEL_MOUNTED))
      m_labelmounted = load_optional_label(m_conf, name(), TAG_LABEL_MOUNTED, "%mountpoint% %percentage_free%");
    if (m_formatter->has(TAG_LABEL_UNMOUNTED))
      m_labelunmounted = load_optional_label(m_conf, name(), TAG_LABEL_UNMOUNTED, "%mountpoint% is not mounted");
    if (m_formatter->has(TAG_BAR_FREE))
      m_barfree = load_progressbar(m_bar, m_conf, name(), TAG_BAR_FREE);
    if (m_formatter->has(TAG_BAR_USED))
      m_barused = load_progressbar(m_bar, m_conf, name(), TAG_BAR_USED);
    if (m_formatter->has(TAG_RAMP_CAPACITY))
      m_rampcapacity = load_ramp(m_conf, name(), TAG_RAMP_CAPACITY);
  }

  /**
   * Update disk values by reading mtab entries
   */
  bool fs_module::update() {
    m_disks.clear();

    struct statvfs buffer;
    struct mntent* mount = nullptr;

    for (auto&& mountpoint : m_mounts) {
      m_disks.emplace_back(new fs_disk{mountpoint, false});

      if (statvfs(mountpoint.c_str(), &buffer) == -1) {
        continue;
      }

      auto mtab = make_unique<mtab_util::reader>();
      auto& disk = m_disks.back();

      while (mtab->next(&mount)) {
        if (strncmp(mount->mnt_dir, mountpoint.c_str(), strlen(mount->mnt_dir)) != 0) {
          continue;
        }

        disk->mounted = true;
        disk->mountpoint = mount->mnt_dir;
        disk->type = mount->mnt_type;
        disk->fsname = mount->mnt_fsname;

        auto b_total = buffer.f_bsize * buffer.f_blocks;
        auto b_free = buffer.f_bsize * buffer.f_bfree;
        auto b_used = b_total - b_free;

        disk->bytes_total = b_total;
        disk->bytes_free = b_free;
        disk->bytes_used = b_used;

        disk->percentage_free = math_util::percentage<unsigned long long, float>(b_free, 0, b_total);
        disk->percentage_used = math_util::percentage<unsigned long long, float>(b_used, 0, b_total);

        disk->percentage_free_s = string_util::floatval(disk->percentage_free, 2, m_fixed, m_bar.locale);
        disk->percentage_used_s = string_util::floatval(disk->percentage_used, 2, m_fixed, m_bar.locale);
      }
    }

    return true;
  }

  /**
   * Generate the module output
   */
  string fs_module::get_output() {
    string output;

    for (m_index = 0; m_index < m_disks.size(); ++m_index) {
      if (!output.empty())
        m_builder->space(m_spacing);
      output += timer_module::get_output();
    }

    return output;
  }

  /**
   * Select format based on fs state
   */
  string fs_module::get_format() const {
    return m_disks[m_index]->mounted ? FORMAT_MOUNTED : FORMAT_UNMOUNTED;
  }

  /**
   * Output content using configured format tags
   */
  bool fs_module::build(builder* builder, string tag) const {
    auto& disk = m_disks[m_index];

    if (tag == TAG_BAR_FREE) {
      builder->node(m_barfree->output(disk->percentage_free));
    } else if (tag == TAG_BAR_USED) {
      builder->node(m_barused->output(disk->percentage_used));
    } else if (tag == TAG_RAMP_CAPACITY) {
      builder->node(m_rampcapacity->get_by_percentage(disk->percentage_free));
    } else if (tag == TAG_LABEL_MOUNTED || tag == TAG_LABEL_UNMOUNTED) {
      label_t label;

      if (tag == TAG_LABEL_MOUNTED)
        label = m_labelmounted->clone();
      else
        label = m_labelunmounted->clone();

      label->reset_tokens();
      label->replace_token("%mountpoint%", disk->mountpoint);
      label->replace_token("%type%", disk->type);
      label->replace_token("%fsname%", disk->fsname);

      label->replace_token("%percentage_free%", disk->percentage_free_s + "%");
      label->replace_token("%percentage_used%", disk->percentage_used_s + "%");

      label->replace_token("%total%", string_util::filesize(disk->bytes_total, 1, m_fixed, m_bar.locale));
      label->replace_token("%free%", string_util::filesize(disk->bytes_free, 2, m_fixed, m_bar.locale));
      label->replace_token("%used%", string_util::filesize(disk->bytes_used, 2, m_fixed, m_bar.locale));

      builder->node(label);
    } else {
      return false;
    }

    return true;
  }
}

LEMONBUDDY_NS_END
