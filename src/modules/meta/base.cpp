#include <utility>

#include "components/builder.hpp"
#include "modules/meta/base.hpp"
#include "drawtypes/label.hpp"

POLYBAR_NS

namespace modules {
  // module_format {{{

  string module_format::decorate(builder* builder, string output) {
    if (offset != 0) {
      builder->offset(offset);
    }
    if (margin > 0) {
      builder->space(margin);
    }
    if (!bg.empty()) {
      builder->background(bg);
    }
    if (!fg.empty()) {
      builder->color(fg);
    }
    if (!ul.empty()) {
      builder->underline(ul);
    }
    if (!ol.empty()) {
      builder->overline(ol);
    }
    if (padding > 0) {
      builder->space(padding);
    }

    if (!output.empty()) {
      builder->node(prefix);
      builder->append(move(output));
      builder->node(suffix);
    }

    if (padding > 0) {
      builder->space(padding);
    }
    if (!ol.empty()) {
      builder->overline_close();
    }
    if (!ul.empty()) {
      builder->underline_close();
    }
    if (!fg.empty()) {
      builder->color_close();
    }
    if (!bg.empty()) {
      builder->background_close();
    }
    if (margin > 0) {
      builder->space(margin);
    }

    return builder->flush();
  }

  // }}}
  // module_formatter {{{

  void module_formatter::add(string name, string fallback, vector<string>&& tags, vector<string>&& whitelist) {
    auto format = make_unique<module_format>();

    format->value = m_conf.get<string>(m_modname, name, move(fallback));
    format->fg = m_conf.get<string>(m_modname, name + "-foreground", "");
    format->bg = m_conf.get<string>(m_modname, name + "-background", "");
    format->ul = m_conf.get<string>(m_modname, name + "-underline", "");
    format->ol = m_conf.get<string>(m_modname, name + "-overline", "");
    format->spacing = m_conf.get<int>(m_modname, name + "-spacing", DEFAULT_SPACING);
    format->padding = m_conf.get<int>(m_modname, name + "-padding", 0);
    format->margin = m_conf.get<int>(m_modname, name + "-margin", 0);
    format->offset = m_conf.get<int>(m_modname, name + "-offset", 0);
    format->tags.swap(tags);

    try {
      format->prefix = load_label(m_conf, m_modname, name + "-prefix");
    } catch (const key_error& err) {
      // prefix not defined
    }

    try {
      format->suffix = load_label(m_conf, m_modname, name + "-suffix");
    } catch (const key_error& err) {
      // suffix not defined
    }

    for (auto&& tag : string_util::split(format->value, ' ')) {
      if (tag[0] != '<' || tag[tag.length() - 1] != '>') {
        continue;
      }
      if (find(format->tags.begin(), format->tags.end(), tag) != format->tags.end()) {
        continue;
      }
      if (find(whitelist.begin(), whitelist.end(), tag) != whitelist.end()) {
        continue;
      }
      throw undefined_format_tag("[" + m_modname + "] Undefined \"" + name + "\" tag: " + tag);
    }

    m_formats.insert(make_pair(name, move(format)));
  }

  bool module_formatter::has(const string& tag, const string& format_name) {
    auto format = m_formats.find(format_name);
    if (format == m_formats.end()) {
      throw undefined_format(format_name);
    }
    return format->second->value.find(tag) != string::npos;
  }

  bool module_formatter::has(const string& tag) {
    for (auto&& format : m_formats) {
      if (format.second->value.find(tag) != string::npos) {
        return true;
      }
    }
    return false;
  }

  shared_ptr<module_format> module_formatter::get(const string& format_name) {
    auto format = m_formats.find(format_name);
    if (format == m_formats.end()) {
      throw undefined_format("Format \"" + format_name + "\" has not been added");
    }
    return format->second;
  }

  // }}}
}

POLYBAR_NS_END
