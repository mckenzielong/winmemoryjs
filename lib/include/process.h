#pragma once
#ifndef PROCESS_H
#define PROCESS_H
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <napi.h>

namespace Process {
  struct Pair {
    HANDLE handle;
    PROCESSENTRY32 process;
  };
  Napi::Value getProcesses(Napi::Env env);
  std::vector<PROCESSENTRY32> getWindowsProcesses(Napi::Env env);
  Napi::Value convertProcessEntryArray(Napi::Env env, const std::vector<PROCESSENTRY32> &processEntries);
  HANDLE openProcess(long processId, std::string* errorMessage);
  void closeProcess(HANDLE hProcess, std::string* errorMessage);
}

#endif
#pragma once
