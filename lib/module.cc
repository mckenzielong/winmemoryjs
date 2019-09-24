#include "module.h"
#include "module_entry.h"

std::vector<MODULEENTRY32> module::getWindowsModules(int processId, Napi::Env env) {
  // Take a snapshot of all modules inside a given process.
  HANDLE hModuleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
  MODULEENTRY32 mEntry;

  if (hModuleSnapshot == INVALID_HANDLE_VALUE) {
    throw Napi::Error::New(env, "method failed to take snapshot of the modules");
  }

  // Before use, set the structure size.
  mEntry.dwSize = sizeof(mEntry);

  // Exit if unable to find the first module.
  if (!Module32First(hModuleSnapshot, &mEntry)) {
    CloseHandle(hModuleSnapshot);
    throw Napi::Error::New(env, "method failed to retrieve the first module");
  }

  std::vector<MODULEENTRY32> modules;

  // Loop through modules.
  do {
    // Add the module to the vector
    modules.push_back(mEntry);
  } while (Module32Next(hModuleSnapshot, &mEntry));

  CloseHandle(hModuleSnapshot);

  return modules;  
}

Napi::Value module::convertModuleEntryArray(Napi::Env env, const std::vector<MODULEENTRY32> &moduleEntries) {
  Napi::Array modules = Napi::Array::New(env);
  uint32_t  i = 0;
  for (const auto &entry : moduleEntries) {
    modules[i++] = MemoryAPI::ModuleEntry::New(env, entry);
  }
  return modules;
}