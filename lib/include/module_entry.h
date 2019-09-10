#pragma once
#ifndef MODULE_ENTRY_H
#define MODULE_ENTRY_H
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <TlHelp32.h>
#include <napi.h>

namespace MemoryAPI {
  class ModuleEntry : public Napi::ObjectWrap<ModuleEntry> {
    public:
      static Napi::Object Init(Napi::Env env, Napi::Object exports);
      ModuleEntry(const Napi::CallbackInfo& info);
      ~ModuleEntry();
      static Napi::Object New(Napi::Env env, const MODULEENTRY32 &entry);

    private: 
      static Napi::FunctionReference constructor;
      int dwSize;
      int th32ProcessID;
      int cntThreads;
      int th32ParentProcessID;
      long pcPriClassBase;
      std::string szExePath;

      Napi::Value getDwSize(const Napi::CallbackInfo &info);
      Napi::Value getModBaseAddr(const Napi::CallbackInfo &info);
      Napi::Value getModBaseSize(const Napi::CallbackInfo &info);
      Napi::Value getTh32ProcessID(const Napi::CallbackInfo &info);
      Napi::Value getSzModule(const Napi::CallbackInfo &info);
      Napi::Value getSzExePath(const Napi::CallbackInfo &info);
  };
}

#endif
#pragma once