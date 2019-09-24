#pragma once
#ifndef MODULE_H
#define MODULE_H
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <napi.h>

namespace module {
  DWORD64 getBaseAddress(const char* processName, DWORD processId);
  MODULEENTRY32 findModule(const char* moduleName, DWORD processId, char** errorMessage);
  std::vector<MODULEENTRY32> getModules(DWORD processId, char** errorMessage);
  std::vector<MODULEENTRY32> getWindowsModules(int processId, Napi::Env env);
  Napi::Value convertModuleEntryArray(Napi::Env env, const std::vector<MODULEENTRY32> &moduleEntries);
  std::vector<THREADENTRY32> getThreads(DWORD processId, char** errorMessage);

};
#endif
#pragma once
