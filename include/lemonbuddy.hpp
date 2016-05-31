#pragma once

#include "exception.hpp"

DefineBaseException(ApplicationError);

void register_pid(pid_t pid);
void unregister_pid(pid_t pid);

void register_command_handler(const std::string& module_name);
