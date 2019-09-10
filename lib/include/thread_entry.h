#pragma once
#ifndef THREAD_ENTRY_H
#define THREAD_ENTRY_H
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <TlHelp32.h>
#include <napi.h>

namespace MemoryAPI {
  class ThreadEntry : public Napi::ObjectWrap<ThreadEntry> {
    public:
      static Napi::Object Init(Napi::Env env, Napi::Object exports);
      ThreadEntry(const Napi::CallbackInfo& info);
      ~ThreadEntry();
      static Napi::Object New(Napi::Env env, const THREADENTRY32 &entry);

    private: 
      static Napi::FunctionReference constructor;
      int dwSize;
      int th32ThreadID;
      int th32OwnerProcessID;
      long tpBasePri;

      Napi::Value getDwSize(const Napi::CallbackInfo &info);
      Napi::Value getTh32ThreadID(const Napi::CallbackInfo &info);
      Napi::Value getTh32OwnerProcessID(const Napi::CallbackInfo &info);
      Napi::Value getTpBasePri(const Napi::CallbackInfo &info);
  };
}

#endif
#pragma once