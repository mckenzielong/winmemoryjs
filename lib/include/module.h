#pragma once
#ifndef MODULE_H
#define MODULE_H
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <napi.h>

namespace module {
  std::vector<MODULEENTRY32> getWindowsModules(int processId, Napi::Env env);
  Napi::Value convertModuleEntryArray(Napi::Env env, const std::vector<MODULEENTRY32> &moduleEntries);
};
#endif
#pragma once
