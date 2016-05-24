#include "config.hpp"
#include "modules/memory.hpp"
#include "utils/config.hpp"

using namespace modules;

MemoryModule::MemoryModule(const std::string& name_) : TimerModule(name_, 1s)
{
  this->interval = std::chrono::duration<double>(
    config::get<float>(name(), "interval", 1));

  this->formatter->add(DEFAULT_FORMAT, TAG_LABEL, { TAG_LABEL, TAG_BAR_USED, TAG_BAR_FREE });

  if (this->formatter->has(TAG_LABEL))
    this->label = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL), "%percentage_used%");
  if (this->formatter->has(TAG_BAR_USED))
    this->bar_used = drawtypes::get_config_bar(name(), get_tag_name(TAG_BAR_USED));
  if (this->formatter->has(TAG_BAR_FREE))
    this->bar_free = drawtypes::get_config_bar(name(), get_tag_name(TAG_BAR_FREE));

  if (this->label)
    this->label_tokenized = this->label->clone();
}

bool MemoryModule::update()
{
  long kbytes_total, kbytes_available/*, kbytes_free*/;

  try {
    std::string str, rdbuf;
    std::ifstream in(PATH_MEMORY_INFO);
    std::stringstream buffer;
    int i = 0;

    in.exceptions(in.failbit);

    buffer.imbue(std::locale::classic());

    while (std::getline(in, str) && i++ < 3) {
      size_t off = str.find_first_of("1234567890", str.find(':'));
      buffer << std::strtol(&str[off], 0, 10) << std::endl;
    }

    buffer >> rdbuf; kbytes_total = std::atol(rdbuf.c_str());
    buffer >> rdbuf; //kbytes_free = std::atol(rdbuf.c_str());
    buffer >> rdbuf; kbytes_available = std::atol(rdbuf.c_str());
  } catch (std::ios_base::failure &e) {
    kbytes_total = 0;
    // kbytes_free = 0;
    kbytes_available = 0;
    log_error("Failed to read memory values: "+ STR(e.what()));
  }

  if (kbytes_total > 0)
    this->percentage_free = ((float) kbytes_available) / kbytes_total * 100.0f + 0.5f;
  else
    this->percentage_free = 0;

  this->percentage_used = 100 - this->percentage_free;

  this->label_tokenized->text = this->label->text;
  this->label_tokenized->replace_token("%percentage_used%", std::to_string(this->percentage_used)+"%");
  this->label_tokenized->replace_token("%percentage_free%", std::to_string(this->percentage_free)+"%");

  auto replace_unit = [](drawtypes::Label *label, const std::string& token, float value, const std::string& unit){
    if (label->text.find(token) != std::string::npos) {
      std::stringstream ss;
      ss.precision(2);
      ss << std::fixed << value;
      label->replace_token(token, ss.str() +" "+ unit);
    }
  };

  replace_unit(this->label_tokenized.get(), "%gb_used%",  (float) (kbytes_total - kbytes_available) / 1024 / 1024, "GB");
  replace_unit(this->label_tokenized.get(), "%gb_free%",  (float) kbytes_available / 1024 / 1024, "GB");
  replace_unit(this->label_tokenized.get(), "%gb_total%", (float) kbytes_total / 1024 / 1024, "GB");

  replace_unit(this->label_tokenized.get(), "%mb_used%",  (float) (kbytes_total - kbytes_available) / 1024, "MB");
  replace_unit(this->label_tokenized.get(), "%mb_free%",  (float) kbytes_available / 1024, "MB");
  replace_unit(this->label_tokenized.get(), "%mb_total%", (float) kbytes_total / 1024, "MB");

  return true;
}

bool MemoryModule::build(Builder *builder, const std::string& tag)
{
  if (tag == TAG_BAR_USED)
    builder->node(this->bar_used, this->percentage_used);
  else if (tag == TAG_BAR_FREE)
    builder->node(this->bar_free, this->percentage_free);
  else if (tag == TAG_LABEL)
    builder->node(this->label_tokenized);
  else
    return false;

  return true;
}
