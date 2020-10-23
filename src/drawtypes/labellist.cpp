#include "drawtypes/labellist.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

namespace drawtypes {
  label_t& labellist::get_template() {
    return m_template;
  }

  void labellist::apply_template() {
    for (label_t& label : m_labels) {
      label->useas_token(m_template, "%label%");
    }
  }
  
  void load_labellist(vector<label_t>& labels, label_t& tmplate, const config& conf, const string& section, string name, bool required) {
    vector<string> names;
    if(required) {
      names = conf.get_list(section, name);
    } else {
      names = conf.get_list(section, name, {});
    }

    tmplate = load_label(conf, section, name, false, "%label%");
    for (size_t i = 0; i < names.size(); i++) {
      labels.emplace_back(forward<label_t>(load_optional_label(conf, section, name + "-" + to_string(i), names[i])));
      labels.back()->copy_undefined(tmplate);
      labels.back()->useas_token(tmplate, "%label%");
    }
  }
}

POLYBAR_NS_END
