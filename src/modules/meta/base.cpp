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
    if (offset) {
      builder->offset(offset);
    }
    if (margin) {
      builder->spacing(margin);
    }
    if (bg.has_color()) {
      builder->background(bg);
    }
    if (fg.has_color()) {
      builder->foreground(fg);
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
    if (padding) {
      builder->spacing(padding);
    }

    builder->node(prefix);

    if (bg.has_color()) {
      builder->background(bg);
    }
    if (fg.has_color()) {
      builder->foreground(fg);
    }
    if (ul.has_color()) {
      builder->underline(ul);
    }
    if (ol.has_color()) {
      builder->overline(ol);
    }

    builder->node(output);
    builder->node(suffix);

    if (padding) {
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
      builder->foreground_close();
    }
    if (bg.has_color()) {
      builder->background_close();
    }
    if (margin) {
      builder->spacing(margin);
    }

    return builder->flush();
  }

  // }}}
  // module_formatter {{{

  void module_formatter::add_value(string&& name, string&& value, vector<string>&& tags, vector<string>&& whitelist) {
    auto format = make_unique<module_format>();
    format->value = move(value);
    // format->fg = m_conf.get(m_modname, name + "-foreground", formatdef("foreground", format->fg));
    format->fg = m_conf["modules"][m_modname][name]["foreground"].as<rgba>(m_conf["settings"]["format"]["foreground"].as<rgba>(format->fg));
    format->bg = m_conf["modules"][m_modname][name]["background"].as<rgba>(m_conf["settings"]["format"]["background"].as<rgba>(format->bg));
    format->ul = m_conf["modules"][m_modname][name]["underline"].as<rgba>(m_conf["settings"]["format"]["underline"].as<rgba>(format->ul));
    format->ol = m_conf["modules"][m_modname][name]["overline"].as<rgba>(m_conf["settings"]["format"]["overline"].as<rgba>(format->ol));
    format->ulsize = m_conf["modules"][m_modname][name]["underline-size"].as<size_t>(m_conf["settings"]["format"]["underline-size"].as<size_t>(format->ulsize));
    format->olsize = m_conf["modules"][m_modname][name]["overline-size"].as<size_t>(m_conf["settings"]["format"]["overline-size"].as<size_t>(format->olsize));
    format->spacing = m_conf["modules"][m_modname][name]["spacing"].as<spacing_val>(m_conf["settings"]["format"]["spacing"].as<spacing_val>(format->spacing));
    format->padding = m_conf["modules"][m_modname][name]["padding"].as<spacing_val>(m_conf["settings"]["format"]["padding"].as<spacing_val>(format->padding));
    format->margin = m_conf["modules"][m_modname][name]["margin"].as<spacing_val>(m_conf["settings"]["format"]["margin"].as<spacing_val>(format->margin));
    format->offset = m_conf["modules"][m_modname][name]["offset"].as<extent_val>(m_conf["settings"]["format"]["offset"].as<extent_val>(format->offset));
    format->font = m_conf["modules"][m_modname][name]["font"].as<int>(m_conf["settings"]["format"]["font"].as<int>(format->font));

    try {
      format->prefix = load_label(m_conf["modules"][m_modname][name]["prefix"]);
    } catch (const key_error& err) {
      // prefix not defined
    }

    try {
      format->suffix = load_label(m_conf["modules"][m_modname][name]["suffix"]);
    } catch (const key_error& err) {
      // suffix not defined
    }

    vector<string> tag_collection;
    tag_collection.reserve(tags.size() + whitelist.size());
    tag_collection.insert(tag_collection.end(), tags.begin(), tags.end());
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
    string value = m_conf["modules"][m_modname][name].as<string>(move(fallback));
    add_value(move(name), move(value), forward<vector<string>>(tags), forward<vector<string>>(whitelist));
  }

  void module_formatter::add_optional(string name, vector<string>&& tags, vector<string>&& whitelist) {
    try {
      string value = m_conf["modules"][m_modname][name].as<string>();
      add_value(move(name), move(value), move(tags), move(whitelist));
    } catch (const key_error&){
      // Optional format is not defined, nothing to do
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
} // namespace modules

POLYBAR_NS_END
