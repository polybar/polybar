#include <sys/statvfs.h>

#include "modules/fs.hpp"

#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"
#include "utils/mtab.hpp"
#include "utils/string.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<fs_module>;

  /**
   * Bootstrap the module by reading config values and
   * setting up required components
   */
  fs_module::fs_module(const bar_settings& bar, string name_) : timer_module<fs_module>(bar, move(name_)) {
    m_mountpoints = m_conf.get_list<string>(name(), "mount");
    m_fixed = m_conf.get<bool>(name(), "fixed-values", m_fixed);
    m_spacing = m_conf.get<int>(name(), "spacing", m_spacing);
    m_interval = chrono::duration<double>(m_conf.get<float>(name(), "interval", 30));

    // Add formats and elements
    m_formatter->add(
        FORMAT_MOUNTED, TAG_LABEL_MOUNTED, {TAG_LABEL_MOUNTED, TAG_BAR_FREE, TAG_BAR_USED, TAG_RAMP_CAPACITY});
    m_formatter->add(FORMAT_UNMOUNTED, TAG_LABEL_UNMOUNTED, {TAG_LABEL_UNMOUNTED});

    if (m_formatter->has(TAG_LABEL_MOUNTED)) {
      m_labelmounted = load_optional_label(m_conf, name(), TAG_LABEL_MOUNTED, "%mountpoint% %percentage_free%");
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
  }

  /**
   * Update values by reading mtab entries
   */
  bool fs_module::update() {
    m_mounts.clear();

    struct statvfs buffer {};
    struct mntent* mnt = nullptr;

    for (auto&& mountpoint : m_mountpoints) {
      m_mounts.emplace_back(new fs_mount{mountpoint, false});

      if (statvfs(mountpoint.c_str(), &buffer) == -1) {
        continue;
      }

      auto mtab = factory_util::unique<mtab_util::reader>();
      auto& mount = m_mounts.back();

      while (mtab->next(&mnt)) {
        if (string{mnt->mnt_dir} != mountpoint) {
          continue;
        }

        mount->mounted = true;
        mount->mountpoint = mnt->mnt_dir;
        mount->type = mnt->mnt_type;
        mount->fsname = mnt->mnt_fsname;

        auto b_total = buffer.f_bsize * buffer.f_blocks;
        auto b_free = buffer.f_bsize * buffer.f_bfree;
        auto b_avail = buffer.f_bsize * buffer.f_bavail;
        auto b_used = b_total - b_avail;

        mount->bytes_total = b_total;
        mount->bytes_free = b_free;
        mount->bytes_used = b_used;

        mount->percentage_free = math_util::percentage<decltype(b_avail)>(b_avail, 0, b_total);
        mount->percentage_used = math_util::percentage<decltype(b_avail)>(b_used, 0, b_total);

        mount->percentage_free_s = string_util::floatval(mount->percentage_free, 2, m_fixed, m_bar.locale);
        mount->percentage_used_s = string_util::floatval(mount->percentage_used, 2, m_fixed, m_bar.locale);
      }
    }

    return true;
  }

  /**
   * Generate the module output
   */
  string fs_module::get_output() {
    string output;

    for (m_index = 0; m_index < m_mounts.size(); ++m_index) {
      if (!output.empty()) {
        m_builder->space(m_spacing);
      }
      output += timer_module::get_output();
    }

    return output;
  }

  /**
   * Select format based on fs state
   */
  string fs_module::get_format() const {
    return m_mounts[m_index]->mounted ? FORMAT_MOUNTED : FORMAT_UNMOUNTED;
  }

  /**
   * Output content using configured format tags
   */
  bool fs_module::build(builder* builder, const string& tag) const {
    auto& mount = m_mounts[m_index];

    if (tag == TAG_BAR_FREE) {
      builder->node(m_barfree->output(mount->percentage_free));
    } else if (tag == TAG_BAR_USED) {
      builder->node(m_barused->output(mount->percentage_used));
    } else if (tag == TAG_RAMP_CAPACITY) {
      builder->node(m_rampcapacity->get_by_percentage(mount->percentage_free));
    } else if (tag == TAG_LABEL_MOUNTED) {
      m_labelmounted->reset_tokens();
      m_labelmounted->replace_token("%mountpoint%", mount->mountpoint);
      m_labelmounted->replace_token("%type%", mount->type);
      m_labelmounted->replace_token("%fsname%", mount->fsname);
      m_labelmounted->replace_token("%percentage_free%", mount->percentage_free_s + "%");
      m_labelmounted->replace_token("%percentage_used%", mount->percentage_used_s + "%");
      m_labelmounted->replace_token("%total%", string_util::filesize(mount->bytes_total, 1, m_fixed, m_bar.locale));
      m_labelmounted->replace_token("%free%", string_util::filesize(mount->bytes_free, 2, m_fixed, m_bar.locale));
      m_labelmounted->replace_token("%used%", string_util::filesize(mount->bytes_used, 2, m_fixed, m_bar.locale));
      builder->node(m_labelmounted);
    } else if (tag == TAG_LABEL_UNMOUNTED) {
      m_labelunmounted->reset_tokens();
      m_labelunmounted->replace_token("%mountpoint%", mount->mountpoint);
      builder->node(m_labelunmounted);
    } else {
      return false;
    }

    return true;
  }
}

POLYBAR_NS_END
