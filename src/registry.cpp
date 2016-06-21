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

Registry::Registry() : logger(get_logger())
{
  this->logger->debug("Entering STAGE 1");
  this->stage = STAGE_1;
}

bool Registry::ready()
{
  auto stage = this->stage();

  if (stage == STAGE_2)
    for (auto &&entry : this->modules)
      if (!entry->warmedup) this->logger->debug("Waiting for: "+ entry->module->name());

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

  this->logger->debug("Entering STAGE 2");

  this->stage = STAGE_2;

  this->logger->debug("Loading modules");

  for (auto &&entry : this->modules) {
    std::lock_guard<std::mutex> lck(this->wait_lock);
    entry->module->start();
  }
}

void Registry::unload()
{
  if (this->stage() != STAGE_3)
    return;

  this->logger->debug("Entering STAGE 4");

  this->stage = STAGE_4;

  this->logger->debug("Unloading modules");

  // Release wait lock
  {
    std::lock_guard<std::mutex> lck(this->wait_lock);
    this->wait_handler.notify_all();
  }

  for (auto &&entry : this->modules) {
    this->logger->debug("Stopping module: "+ entry->module->name());
    entry->module->stop();
  }
}

bool Registry::wait()
{
  log_trace("STAGE "+ std::to_string(this->stage()));

  std::unique_lock<std::mutex> lck(this->wait_lock);

  auto stage = this->stage();

  if (stage < STAGE_2)
    return false;

  else if (stage == STAGE_2)
    while (stage == STAGE_2) {
      bool ready = true;

      for (auto &&entry : this->modules)
        if (!entry->warmedup) ready = false;

      if (!ready) {
        this->wait_handler.wait(lck);
        continue;
      }

      this->logger->debug("Received initial broadcast from all modules");
      this->logger->debug("Entering STAGE 3");

      this->stage = STAGE_3;
      break;
    }

  else if (stage == STAGE_3)
    this->wait_handler.wait(lck);

  else if (stage == STAGE_4)
    this->modules.clear();

  return true;
}

void Registry::notify(std::string module_name)
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

  std::unique_lock<std::mutex> lck(this->wait_lock);

  try {
    this->logger->debug("Refreshing output for module: "+ module_name);
    mod_entry->module->refresh();
  } catch (Exception &e) {
    log_trace("Exception occurred in runner thread for: "+ module_name);
    this->logger->error(e.what());
  }

  lck.unlock();

  this->wait_handler.notify_one();
}

std::string Registry::get(std::string module_name)
{
  return (*this->find(module_name)->module)();
}

std::unique_ptr<RegistryModuleEntry>& Registry::find(std::string module_name)
{
  for (auto &&entry : this->modules)
    if (entry->module->name() == module_name)
      return entry;
  throw ModuleNotFound(module_name);
}
