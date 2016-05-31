#include "registry.hpp"
#include "services/logger.hpp"
#include "utils/string.hpp"

std::shared_ptr<Registry> registry;
std::shared_ptr<Registry> &get_registry()
{
  if (registry == nullptr)
    registry = std::make_shared<Registry>();
  return registry;
}

Registry::Registry()
{
  get_logger()->debug("Entering STAGE 1");
  this->stage = STAGE_1;
}

bool Registry::ready()
{
  auto stage = this->stage();

  if (stage == STAGE_2)
    for (auto &&entry : this->modules)
      if (!entry->warmedup) get_logger()->debug("Waiting for: "+ entry->module->name());

  return stage == STAGE_3;
}

void Registry::insert(std::unique_ptr<modules::ModuleInterface> &&module)
{
  log_trace("Inserting module: "+ module->name());
  this->modules.emplace_back(std::make_unique<RegistryModuleEntry>(std::move(module)));
}

void Registry::load()
{
  if (this->stage() != STAGE_1)
    return;

  get_logger()->debug("Entering STAGE 2");

  this->stage = STAGE_2;

  get_logger()->debug("Loading modules");

  for (auto &&entry : this->modules) {
    std::lock_guard<std::mutex> wait_lck(this->wait_mtx);
    entry->module->start();
  }
}

void Registry::unload()
{
  if (this->stage() != STAGE_3)
    return;

  get_logger()->debug("Entering STAGE 4");

  this->stage = STAGE_4;

  get_logger()->debug("Unloading modules");

  // Release wait lock
  {
    std::lock_guard<std::mutex> wait_lck(this->wait_mtx);
    this->wait_cv.notify_one();
  }

  for (auto &&entry : this->modules)
    entry->module->stop();
}

bool Registry::wait()
{
  log_trace("STAGE "+ std::to_string(this->stage()));

  std::unique_lock<std::mutex> wait_lck(this->wait_mtx);

  auto stage = this->stage();

  if (stage < STAGE_2)
    return false;

  else if (stage == STAGE_2)
    while (stage == STAGE_2) {
      bool ready = true;

      for (auto &&entry : this->modules)
        if (!entry->warmedup) ready = false;

      if (!ready) {
        this->wait_cv.wait(wait_lck);
        continue;
      }

      get_logger()->info("Received initial broadcast from all modules");
      get_logger()->debug("Entering STAGE 3");

      this->stage = STAGE_3;
      break;
    }

  else if (stage == STAGE_3)
    this->wait_cv.wait(wait_lck);

  else if (stage == STAGE_4)
    this->modules.clear();

  return true;
}

void Registry::notify(const std::string& module_name)
{
  log_trace(module_name +" - STAGE "+ std::to_string(this->stage()));

  auto stage = this->stage();

  if (stage == STAGE_4)
    return;

  auto &mod_entry = this->find(module_name);

  if (stage == STAGE_2) {
    if (mod_entry->warmedup())
      while (this->stage() == STAGE_2)
        std::this_thread::sleep_for(100ms);
    else
      mod_entry->warmedup = true;
  }

  std::unique_lock<std::mutex> wait_lck(this->wait_mtx);

  try {
    mod_entry->module->refresh();
  } catch (Exception &e) {
    log_trace("Exception occurred in runner thread for: "+ module_name);
    get_logger()->error(e.what());
  }


  wait_lck.unlock();

  this->wait_cv.notify_one();
}

std::string Registry::get(const std::string& module_name)
{
  return (*this->find(module_name)->module)();
}

std::unique_ptr<RegistryModuleEntry>& Registry::find(const std::string& module_name)
{
  for (auto &&entry : this->modules)
    if (entry->module->name() == module_name)
      return entry;
  throw ModuleNotFound(module_name);
}
