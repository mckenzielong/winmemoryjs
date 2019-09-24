#include "thread.h"
#include "thread_entry.h"

std::vector<THREADENTRY32> Thread::getWindowsThreads(int processId, Napi::Env env) {
  HANDLE hThreadSnapshot = INVALID_HANDLE_VALUE;
  hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

  THREADENTRY32 mEntry;

  if (hThreadSnapshot == INVALID_HANDLE_VALUE) {
    throw Napi::Error::New(env, "method failed to take snapshot of the threads");
  }

  mEntry.dwSize = sizeof(mEntry);

  if(!Thread32First(hThreadSnapshot, &mEntry)) {
    CloseHandle(hThreadSnapshot);
    throw Napi::Error::New(env, "method failed to retrieve the first thread");
  }

  std::vector<THREADENTRY32> threads;
  do {
    if (mEntry.th32OwnerProcessID == processId) {
      threads.push_back(mEntry);
    }
  } while (Thread32Next(hThreadSnapshot, &mEntry));

  CloseHandle(hThreadSnapshot);

  return threads;
}

Napi::Value Thread::convertThreadEntryArray(Napi::Env env, const std::vector<THREADENTRY32> &threadEntries) {
  Napi::Array threads = Napi::Array::New(env);
  uint32_t  i = 0;
  for (auto const &entry : threadEntries) {
    threads[i++] = MemoryAPI::ThreadEntry::New(env, entry);
  }
  return threads;
}
