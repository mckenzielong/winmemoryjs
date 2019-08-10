#pragma once
#ifndef PROCESS_ENTRY_H
#define PROCESS_ENTRY_H
#define WIN32_LEAN_AND_MEAN
#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <TlHelp32.h>
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
      int th32ProcessID;
      int cntThreads;
      int th32ParentProcessID;
      long pcPriClassBase;
      std::string szExeFile;

      Napi::Value getDwSize(const Napi::CallbackInfo &info);
      Napi::Value getTh32ProcessID(const Napi::CallbackInfo &info);
      Napi::Value getCntThreads(const Napi::CallbackInfo &info);
      Napi::Value getTh32ParentProcessID(const Napi::CallbackInfo &info);
      Napi::Value getPcPriClassBase(const Napi::CallbackInfo &info);
      Napi::Value getSzExeFile(const Napi::CallbackInfo &info);
  };
}

#endif
#pragma once