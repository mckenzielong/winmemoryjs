#pragma once
#ifndef THREAD_H
#define THREAD_H
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <napi.h>

namespace Thread {
  std::vector<THREADENTRY32> getWindowsThreads(int processId, Napi::Env env);
  Napi::Value convertThreadEntryArray(Napi::Env env, const std::vector<THREADENTRY32> &threadEntries);
};
#endif
#pragma once
