#include "drawtypes/labellist.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

namespace drawtypes {
  void labellist::reset_tokens() {
    for(auto label : m_labels)
      label->reset_tokens();
  }

  void labellist::replace_token(const string& token, const string& replacement) {
    for(auto label : m_labels)
      label->replace_token(token, replacement);
  }
  
  void load_labellist(vector<label_t>& labels, const config& conf, const string& section, string name, bool required) {
    vector<string> names;
    if(required) {
      names = conf.get_list(section, name);
    } else {
      names = conf.get_list(section, name, {});
    }

    auto tmplate = load_label(conf, section, name, false, "");
    for (size_t i = 0; i < names.size(); i++) {
      labels.emplace_back(forward<label_t>(load_optional_label(conf, section, name + "-" + to_string(i), names[i])));
      labels.back()->copy_undefined(tmplate);
    }
  }
}

POLYBAR_NS_END
