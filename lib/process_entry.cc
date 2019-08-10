#include <windows.h>
#include <TlHelp32.h>
#include <napi.h>
#include "process_entry.h"

using namespace MemoryAPI;

Napi::FunctionReference ProcessEntry::constructor;

Napi::Object ProcessEntry::New(Napi::Env env, const PROCESSENTRY32 &entry) {
    auto convertedEntry = Napi::External<PROCESSENTRY32>::New(env, &static_cast<PROCESSENTRY32>(entry));
    return constructor.New({convertedEntry});
}

Napi::Object ProcessEntry::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "ProcessEntry", {
        InstanceAccessor("dwSize", &getDwSize, NULL)
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("ProcessEntry", func);
    return exports;
}

ProcessEntry::ProcessEntry(const Napi::CallbackInfo& info) : Napi::ObjectWrap<ProcessEntry>(info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1 && !info[0].IsExternal()) {
    throw Napi::TypeError::New(env, "Must be of type PROCESSENTRY32");
  }

  auto entry = info[0].As<Napi::External<PROCESSENTRY32>>().Data();
  dwSize = entry->dwSize;
}

Napi::Value ProcessEntry::getDwSize(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  return Napi::Number::New(env, dwSize);
}

