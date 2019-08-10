#pragma once
#ifndef PROCESS_ENTRY_H
#define PROCESS_ENTRY_H
#define WIN32_LEAN_AND_MEAN

#include <TlHelp32.h>
#include <cstdint>
#include <napi.h>

namespace MemoryAPI {
  class ProcessEntry : public Napi::ObjectWrap<ProcessEntry> {
    public:
      static Napi::Object Init(Napi::Env env, Napi::Object exports);
      ProcessEntry(const Napi::CallbackInfo& info);
      static Napi::Object New(Napi::Env env, const PROCESSENTRY32 &entry);

    private: 
      static Napi::FunctionReference constructor;
      int dwSize;
      int cntUsage;
      int th32ProcessID;
      uintptr_t th32DefaultHeapID;
      int th32ModuleID;
      int cntThreads;
      int th32ParentProcessID;
      long pcPriClassBase;
      int dwFlags;
      char* szExeFile;
      
      
      Napi::Value getDwSize(const Napi::CallbackInfo &info);
  };
}

#endif
#pragma once