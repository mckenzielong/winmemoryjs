#include "process_entry.h"
#include "process.h"

using namespace MemoryAPI;

Napi::FunctionReference ProcessEntry::constructor;

//Return a new object wrapped entry
Napi::Object ProcessEntry::New(Napi::Env env, const PROCESSENTRY32 &entry) {
    auto convertedEntry = Napi::External<PROCESSENTRY32>::New(env, &static_cast<PROCESSENTRY32>(entry));
    return constructor.New({convertedEntry});
}

ProcessEntry::ProcessEntry(const Napi::CallbackInfo& info) : Napi::ObjectWrap<ProcessEntry>(info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1 && !info[0].IsExternal()) {
    throw Napi::TypeError::New(env, "Must be of type PROCESSENTRY32");
  }

  PROCESSENTRY32* entry = info[0].As<Napi::External<PROCESSENTRY32>>().Data();
  dwSize = entry->dwSize;
  th32ProcessID = entry->th32ProcessID;
  cntThreads = entry->cntThreads;
  th32ParentProcessID = entry->th32ParentProcessID;
  pcPriClassBase = entry->pcPriClassBase;
  szExeFile = std::string(entry->szExeFile);
  handle = NULL;
}

//Object definition and accessor methods
Napi::Object ProcessEntry::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "ProcessEntry", {
        InstanceAccessor("dwSize", &getDwSize, NULL),
        InstanceAccessor("th32ProcessID", &getTh32ProcessID, NULL),
        InstanceAccessor("cntThreads", &getCntThreads, NULL),
        InstanceAccessor("th32ParentProcessID", &getTh32ParentProcessID, NULL),
        InstanceAccessor("pcPriClassBase", &getPcPriClassBase, NULL),
        InstanceAccessor("szExeFile", &getSzExeFile, NULL),
        InstanceMethod("openProcess", &openProcess),
        InstanceMethod("closeProcess", &closeProcess),
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("ProcessEntry", func);
    return exports;
}

Napi::Value ProcessEntry::getDwSize(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), dwSize);
}

Napi::Value ProcessEntry::getTh32ProcessID(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), th32ProcessID);
}

Napi::Value ProcessEntry::getCntThreads(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), cntThreads);
}

Napi::Value ProcessEntry::getTh32ParentProcessID(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), th32ParentProcessID);
}

Napi::Value ProcessEntry::getPcPriClassBase(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), pcPriClassBase);
}

Napi::Value ProcessEntry::getSzExeFile(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), szExeFile);
}

Napi::Value ProcessEntry::openProcess(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() > 0) {
    throw Napi::TypeError::New(env, "openProcess takes no arguments");
  }
  std::string err = "";
  handle = static_cast<void *>(Process::openProcess(th32ProcessID, &err));

  if (!err.empty()) {
    throw Napi::TypeError::New(env, "unable to open process");
  }

  return Napi::Number::New(env, reinterpret_cast<uintptr_t>(handle));
}

Napi::Value ProcessEntry::closeProcess(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() > 0) {
    throw Napi::TypeError::New(env, "closeProcess takes no arguments");
  } else if (!handle) {
    throw Napi::TypeError::New(env, "process is not open");
  }

  std::string err = "";
  Process::closeProcess(handle, &err);

  if (!err.empty()) {
    throw Napi::TypeError::New(env, "unable to close process");
  }
  
  handle = NULL;
  return env.Null();
}

ProcessEntry::~ProcessEntry() {
  if (handle) {
    std::string err = "";
    Process::closeProcess(handle, &err);
  }
}
