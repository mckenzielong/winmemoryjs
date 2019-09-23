#include "thread_entry.h"

using namespace MemoryAPI;

Napi::FunctionReference ThreadEntry::constructor;

//Return a new object wrapped entry
Napi::Object ThreadEntry::New(Napi::Env env, const THREADENTRY32 &entry) {
  auto convertedEntry = Napi::External<THREADENTRY32>::New(env, const_cast<THREADENTRY32 *>(&entry));
  return ThreadEntry::constructor.New({convertedEntry});
}

ThreadEntry::ThreadEntry(const Napi::CallbackInfo& info) : Napi::ObjectWrap<ThreadEntry>(info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1 && !info[0].IsExternal()) {
    throw Napi::TypeError::New(env, "Must be of type THREADENTRY32");
  }

  THREADENTRY32* entry = info[0].As<Napi::External<THREADENTRY32>>().Data();
  dwSize = entry->dwSize;
  th32ThreadID = entry->th32ThreadID;
  th32OwnerProcessID = entry->th32OwnerProcessID;
  tpBasePri = entry->tpBasePri;
  tpDeltaPri = entry->tpDeltaPri;
}

//Object definition and accessor methods
Napi::Object ThreadEntry::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "ThreadEntry", {
        InstanceAccessor("dwSize", &getDwSize, NULL),
        InstanceAccessor("th32ThreadID", &getTh32ThreadID, NULL),
        InstanceAccessor("th32OwnerProcessID", &getTh32OwnerProcessID, NULL),
        InstanceAccessor("tpBasePri", &getTpBasePri, NULL),
        InstanceAccessor("tpDeltaPri", &getTpDeltaPri, NULL)
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("ThreadEntry", func);
    return exports;
}

Napi::Value ThreadEntry::getDwSize(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), dwSize);
}

Napi::Value ThreadEntry::getTh32ThreadID(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), th32ThreadID);
}

Napi::Value ThreadEntry::getTh32OwnerProcessID(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), th32OwnerProcessID);
}

Napi::Value ThreadEntry::getTpBasePri(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), tpBasePri);
}

Napi::Value ThreadEntry::getTpDeltaPri(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), tpDeltaPri);
}


ThreadEntry::~ThreadEntry() {
  //todo
}
