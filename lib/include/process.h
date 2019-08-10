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
  Pair openProcess(const char* processName, char** errorMessage);
  Pair openProcess(DWORD processId, char** errorMessage);
  void closeProcess(HANDLE hProcess);

  class GetProcessAsync : public Napi::AsyncWorker {
    public:    
      GetProcessAsync(const Napi::Env& env, const Napi::Promise::Deferred& promise) : AsyncWorker(env), promise(promise) {};
      GetProcessAsync(const Napi::Function& callback, const Napi::Promise::Deferred& promise) : AsyncWorker(callback), promise(promise) {};
      ~GetProcessAsync() {};

      void Execute();
      void OnOK();
      void OnError(const Napi::Error& e);

    private:
      Napi::Promise::Deferred promise;
      std::vector<PROCESSENTRY32> processes;
  };
}

#endif
#pragma once
