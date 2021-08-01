#include "modules/meta/base.hpp"

#include <utility>

#include "components/builder.hpp"
#include "drawtypes/label.hpp"

POLYBAR_NS

namespace modules {
  // module_format {{{

  string module_format::decorate(builder* builder, string output) {
    if (output.empty()) {
      builder->flush();
      return "";
    }
    if (offset.value != 0) {
      builder->offset(offset);
    }
    if (margin.value > 0) {
      builder->spacing(margin);
    }
    if (bg.has_color()) {
      builder->background(bg);
    }
    if (fg.has_color()) {
      builder->color(fg);
    }
    if (ul.has_color()) {
      builder->underline(ul);
    }
    if (ol.has_color()) {
      builder->overline(ol);
    }
    if (font > 0) {
      builder->font(font);
    }
    if (padding.value > 0) {
      builder->spacing(padding);
    }

    builder->node(prefix);

    if (bg.has_color()) {
      builder->background(bg);
    }
    if (fg.has_color()) {
      builder->color(fg);
    }
    if (ul.has_color()) {
      builder->underline(ul);
    }
    if (ol.has_color()) {
      builder->overline(ol);
    }

    builder->append(move(output));
    builder->node(suffix);

    if (padding.value > 0) {
      builder->spacing(padding);
    }
    if (font > 0) {
      builder->font_close();
    }
    if (ol.has_color()) {
      builder->overline_close();
    }
    if (ul.has_color()) {
      builder->underline_close();
    }
    if (fg.has_color()) {
      builder->color_close();
    }
    if (bg.has_color()) {
      builder->background_close();
    }
    if (margin.value > 0) {
      builder->spacing(margin);
    }

    return builder->flush();
  }

  // }}}
  // module_formatter {{{

  void module_formatter::add_value(string&& name, string&& value, vector<string>&& tags, vector<string>&& whitelist) {
    const auto formatdef = [&](const string& param, const auto& fallback) {
      return m_conf.get("settings", "format-" + param, fallback);
    };

    auto format = make_unique<module_format>();
    format->value = move(value);
    format->fg = m_conf.get(m_modname, name + "-foreground", formatdef("foreground", format->fg));
    format->bg = m_conf.get(m_modname, name + "-background", formatdef("background", format->bg));
    format->ul = m_conf.get(m_modname, name + "-underline", formatdef("underline", format->ul));
    format->ol = m_conf.get(m_modname, name + "-overline", formatdef("overline", format->ol));
    format->ulsize = m_conf.get(m_modname, name + "-underline-size", formatdef("underline-size", format->ulsize));
    format->olsize = m_conf.get(m_modname, name + "-overline-size", formatdef("overline-size", format->olsize));
    format->spacing = m_conf.get(m_modname, name + "-spacing", formatdef("spacing", format->spacing));
    format->padding = m_conf.get(m_modname, name + "-padding", formatdef("padding", format->padding));
    format->margin = m_conf.get(m_modname, name + "-margin", formatdef("margin", format->margin));
    format->offset = m_conf.get(m_modname, name + "-offset", formatdef("offset", format->offset));
    format->font = m_conf.get(m_modname, name + "-font", formatdef("font", format->font));
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

    vector<string> tag_collection;
    tag_collection.reserve(format->tags.size() + whitelist.size());
    tag_collection.insert(tag_collection.end(), format->tags.begin(), format->tags.end());
    tag_collection.insert(tag_collection.end(), whitelist.begin(), whitelist.end());

    size_t start, end;
    while ((start = value.find('<')) != string::npos && (end = value.find('>', start)) != string::npos) {
      if (start > 0) {
        value.erase(0, start);
        end -= start;
        start = 0;
      }
      string tag{value.substr(start, end + 1)};
      if (find(tag_collection.begin(), tag_collection.end(), tag) == tag_collection.end()) {
        throw undefined_format_tag(tag + " is not a valid format tag for \"" + name + "\"");
      }
      value.erase(0, tag.size());
    }

    m_formats.insert(make_pair(move(name), move(format)));
  }

  void module_formatter::add(string name, string fallback, vector<string>&& tags, vector<string>&& whitelist) {
    string value = m_conf.get(m_modname, name, move(fallback));
    add_value(move(name), move(value), forward<vector<string>>(tags), forward<vector<string>>(whitelist));
  }

  void module_formatter::add_optional(string name, vector<string>&& tags, vector<string>&& whitelist) {
    if (m_conf.has(m_modname, name)) {
      string value = m_conf.get(m_modname, name);
      add_value(move(name), move(value), move(tags), move(whitelist));
    }
  }

  bool module_formatter::has(const string& tag, const string& format_name) {
    auto format = m_formats.find(format_name);
    if (format == m_formats.end()) {
      return false;
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

  bool module_formatter::has_format(const string& format_name) {
    return m_formats.find(format_name) != m_formats.end();
  }

  shared_ptr<module_format> module_formatter::get(const string& format_name) {
    auto format = m_formats.find(format_name);
    if (format == m_formats.end()) {
      throw undefined_format("Format \"" + format_name + "\" has not been added");
    }
    return format->second;
  }

  // }}}
}  // namespace modules

POLYBAR_NS_END
