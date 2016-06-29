#include "lemonbuddy.hpp"
#include "bar.hpp"
#include "utils/math.hpp"
#include "utils/macros.hpp"
#include "modules/volume.hpp"

using namespace modules;

VolumeModule::VolumeModule(std::string name_) : EventModule(name_)
{
  // Load configuration values {{{
  auto master_mixer = config::get<std::string>(name(), "master_mixer", "Master");
  auto speaker_mixer = config::get<std::string>(name(), "speaker_mixer", "");
  auto headphone_mixer = config::get<std::string>(name(), "headphone_mixer", "");

  this->headphone_ctrl_numid = config::get<int>(name(), "headphone_control_numid", -1);

  if (!headphone_mixer.empty() && this->headphone_ctrl_numid == -1)
    throw ModuleError("[VolumeModule] Missing required property value for \"headphone_control_numid\"...");
  else if (headphone_mixer.empty() && this->headphone_ctrl_numid != -1)
    throw ModuleError("[VolumeModule] Missing required property value for \"headphone_mixer\"...");

  if (string::lower(speaker_mixer) == "master")
    throw ModuleError("[VolumeModule] The \"Master\" mixer is already processed internally. Specify another mixer or comment out the \"speaker_mixer\" parameter...");
  if (string::lower(headphone_mixer) == "master")
    throw ModuleError("[VolumeModule] The \"Master\" mixer is already processed internally. Specify another mixer or comment out the \"headphone_mixer\" parameter...");
  // }}}

  // Setup mixers {{{
  auto create_mixer = [](std::string mixer_name)
  {
    std::unique_ptr<alsa::Mixer> mixer;

    try {
      mixer = std::make_unique<alsa::Mixer>(mixer_name);
    } catch (alsa::MixerError &e) {
      log_error("Failed to open \""+ mixer_name +"\" mixer => "+ ToStr(e.what()));
      mixer.reset();
    }

    return mixer;
  };

  this->master_mixer = create_mixer(master_mixer);

  if (!speaker_mixer.empty())
    this->speaker_mixer = create_mixer(speaker_mixer);
  if (!headphone_mixer.empty())
    this->headphone_mixer = create_mixer(headphone_mixer);

  if (!this->master_mixer && !this->speaker_mixer && !this->headphone_mixer) {
    this->stop();
    return;
  }

  if (this->headphone_mixer && this->headphone_ctrl_numid > -1) {
    try {
      this->headphone_ctrl = std::make_unique<alsa::ControlInterface>(this->headphone_ctrl_numid);
    } catch (alsa::ControlInterfaceError &e) {
      log_error("Failed to open headphone control interface => "+ ToStr(e.what()));
      this->headphone_ctrl.reset();
    }
  }
  // }}}

  // Add formats and elements {{{
  this->formatter->add(FORMAT_VOLUME, TAG_LABEL_VOLUME,
    { TAG_RAMP_VOLUME, TAG_LABEL_VOLUME, TAG_BAR_VOLUME });
  this->formatter->add(FORMAT_MUTED, TAG_LABEL_MUTED,
    { TAG_RAMP_VOLUME, TAG_LABEL_MUTED, TAG_BAR_VOLUME });

  if (this->formatter->has(TAG_BAR_VOLUME))
    this->bar_volume = drawtypes::get_config_bar(name(), get_tag_name(TAG_BAR_VOLUME));
  if (this->formatter->has(TAG_RAMP_VOLUME))
    this->ramp_volume = drawtypes::get_config_ramp(name(), get_tag_name(TAG_RAMP_VOLUME));
  if (this->formatter->has(TAG_LABEL_VOLUME, FORMAT_VOLUME)) {
    this->label_volume = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_VOLUME), "%percentage%");
    this->label_volume_tokenized = this->label_volume->clone();
  }
  if (this->formatter->has(TAG_LABEL_MUTED, FORMAT_MUTED)) {
    this->label_muted = drawtypes::get_optional_config_label(name(), get_tag_name(TAG_LABEL_MUTED), "%percentage%");
    this->label_muted_tokenized = this->label_muted->clone();
  }
  // }}}
}

VolumeModule::~VolumeModule()
{
  std::lock_guard<concurrency::SpinLock> lck(this->update_lock);
  this->master_mixer.reset();
  this->speaker_mixer.reset();
  this->headphone_mixer.reset();
  this->headphone_ctrl.reset();
}

bool VolumeModule::has_event()
{
  bool has_event = false;

  if (this->has_changed())
    has_event = true;

  try {
    if (!has_event && this->master_mixer)
      has_event |= this->master_mixer->wait(25);
    if (!has_event && this->speaker_mixer)
      has_event |= this->speaker_mixer->wait(25);
    if (!has_event && this->headphone_mixer)
      has_event |= this->headphone_mixer->wait(25);
    if (!has_event && this->headphone_ctrl)
      has_event |= this->headphone_ctrl->wait(25);
  } catch (alsa::Exception &e) {
    log_error(e.what());
  }

  return has_event;
}

bool VolumeModule::update()
{
  // Consume any other pending events
  this->has_changed = false;
  if (this->master_mixer)
    this->master_mixer->process_events();
  if (this->speaker_mixer)
    this->speaker_mixer->process_events();
  if (this->headphone_mixer)
    this->headphone_mixer->process_events();
  if (this->headphone_ctrl)
    this->headphone_ctrl->wait(0);

  int volume = 100;
  bool muted = false;

  if (this->master_mixer) {
    volume *= this->master_mixer->get_volume() / 100.0f;
    muted |= this->master_mixer->is_muted();
  }

  if (this->headphone_mixer && this->headphone_ctrl && this->headphone_ctrl->test_device_plugged()) {
    volume *= this->headphone_mixer->get_volume() / 100.0f;
    muted |= this->headphone_mixer->is_muted();
  } else if (this->speaker_mixer) {
    volume *= this->speaker_mixer->get_volume() / 100.0f;
    muted |= this->speaker_mixer->is_muted();
  }

  this->volume = volume;
  this->muted = muted;

  this->label_volume_tokenized->text = this->label_volume->text;
  this->label_volume_tokenized->replace_token("%percentage%", std::to_string(this->volume()) +"%");

  this->label_muted_tokenized->text = this->label_muted->text;
  this->label_muted_tokenized->replace_token("%percentage%", std::to_string(this->volume()) +"%");

  return true;
}

std::string VolumeModule::get_format()
{
  return this->muted() == true ? FORMAT_MUTED : FORMAT_VOLUME;
}

std::string VolumeModule::get_output()
{
  this->builder->cmd(Cmd::LEFT_CLICK, EVENT_TOGGLE_MUTE);

  if (!this->muted()) {
    if (this->volume() < 100)
      this->builder->cmd(Cmd::SCROLL_UP, EVENT_VOLUME_UP);
    if (this->volume() > 0)
      this->builder->cmd(Cmd::SCROLL_DOWN, EVENT_VOLUME_DOWN);
  }

  this->builder->node(this->Module::get_output());

  return this->builder->flush();
}

bool VolumeModule::build(Builder *builder, std::string tag)
{
  if (tag == TAG_BAR_VOLUME)
    builder->node(this->bar_volume, volume);
  else if (tag == TAG_RAMP_VOLUME)
    builder->node(this->ramp_volume, volume);
  else if (tag == TAG_LABEL_VOLUME)
    builder->node(this->label_volume_tokenized);
  else if (tag == TAG_LABEL_MUTED)
    builder->node(this->label_muted_tokenized);
  else
    return false;

  return true;
}

bool VolumeModule::handle_command(std::string cmd)
{
  if (cmd.length() < std::strlen(EVENT_PREFIX))
    return false;
  if (std::strncmp(cmd.c_str(), EVENT_PREFIX, 3) != 0)
    return false;

  std::lock_guard<concurrency::SpinLock> lck(this->update_lock);

  alsa::Mixer *master_mixer = nullptr;
  alsa::Mixer *other_mixer = nullptr;

  if (this->master_mixer)
    master_mixer = this->master_mixer.get();

  if (master_mixer == nullptr)
    return false;

  if (this->headphone_mixer && this->headphone_ctrl && this->headphone_ctrl->test_device_plugged())
    other_mixer = this->headphone_mixer.get();
  else if (this->speaker_mixer)
    other_mixer = this->speaker_mixer.get();

  // Toggle mute state
  if (std::strncmp(cmd.c_str(), EVENT_TOGGLE_MUTE, std::strlen(EVENT_TOGGLE_MUTE)) == 0) {
    master_mixer->set_mute(this->muted());

    if (other_mixer != nullptr)
      other_mixer->set_mute(this->muted());

  // Increase volume
  } else if (std::strncmp(cmd.c_str(), EVENT_VOLUME_UP, std::strlen(EVENT_VOLUME_UP)) == 0) {
    master_mixer->set_volume(math::cap<float>(master_mixer->get_volume() + 5, 0, 100));

    if (other_mixer != nullptr)
      other_mixer->set_volume(math::cap<float>(other_mixer->get_volume() + 5, 0, 100));

  // Decrease volume
  } else if (std::strncmp(cmd.c_str(), EVENT_VOLUME_DOWN, std::strlen(EVENT_VOLUME_DOWN)) == 0) {
    master_mixer->set_volume(math::cap<float>(master_mixer->get_volume() - 5, 0, 100));

    if (other_mixer != nullptr)
      other_mixer->set_volume(math::cap<float>(other_mixer->get_volume() - 5, 0, 100));

  } else {
    return false;
  }

  this->has_changed = true;

  return true;
}
