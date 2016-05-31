#pragma once

#include "modules/base.hpp"

DefineBaseException(RegistryError);
DefineChildException(ModuleNotFound, RegistryError);

struct RegistryModuleEntry
{
  concurrency::Atomic<bool> warmedup;
  std::unique_ptr<modules::ModuleInterface> module;

  RegistryModuleEntry(std::unique_ptr<modules::ModuleInterface> &&module) : warmedup(false)
  {
    this->module.swap(module);
  }
};

class Registry
{
  // Stopped and no loaded modules
  const int STAGE_1 = 1;
  // Modules loaded but waiting for initial broadcast
  const int STAGE_2 = 2;
  // Running
  const int STAGE_3 = 3;
  // Unloading modules
  const int STAGE_4 = 4;

  concurrency::Atomic<int> stage;

  std::vector<std::unique_ptr<RegistryModuleEntry>> modules;

  std::mutex wait_mtx;
  std::condition_variable wait_cv;

  public:
    Registry();

    bool ready();
    void insert(std::unique_ptr<modules::ModuleInterface> &&module);
    void load();
    void unload();
    bool wait();
    void notify(const std::string& module_name);
    std::string get(const std::string& module_name);
    std::unique_ptr<RegistryModuleEntry>& find(const std::string& module_name);
};

std::shared_ptr<Registry> &get_registry();
