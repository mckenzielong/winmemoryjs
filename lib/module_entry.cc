#include "module_entry.h"

using namespace MemoryAPI;

Napi::FunctionReference ModuleEntry::constructor;

//Return a new object wrapped entry
Napi::Object ModuleEntry::New(Napi::Env env, const MODULEENTRY32 &entry) {
    auto convertedEntry = Napi::External<MODULEENTRY32>::New(env, &static_cast<MODULEENTRY32>(entry));
    return constructor.New({convertedEntry});
}

ModuleEntry::ModuleEntry(const Napi::CallbackInfo& info) : Napi::ObjectWrap<ModuleEntry>(info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1 && !info[0].IsExternal()) {
    throw Napi::TypeError::New(env, "Must be of type MODULEENTRY32");
  }

  MODULEENTRY32* entry = info[0].As<Napi::External<MODULEENTRY32>>().Data();
  dwSize = entry->dwSize;
  th32ProcessID = entry->th32ProcessID;
  szExePath = std::string(entry->szExePath);
  szModule = std::string(entry->szModule);
  modBaseAddr = entry->modBaseAddr;
  modBaseSize = entry->modBaseSize;
}

//Object definition and accessor methods
Napi::Object ModuleEntry::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "ModuleEntry", {
        InstanceAccessor("dwSize", &getDwSize, NULL),
        InstanceAccessor("szExePath", &getSzExePath, NULL),
        InstanceAccessor("modBaseAddr", &getModBaseAddr, NULL),
        InstanceAccessor("modBaseSize", &getModBaseSize, NULL),
        InstanceAccessor("th32ProcessID", &getTh32ProcessID, NULL),
        InstanceAccessor("szModule", &getSzModule, NULL)
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("ModuleEntry", func);
    return exports;
}

Napi::Value ModuleEntry::getDwSize(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), dwSize);
}

Napi::Value ModuleEntry::getSzExePath(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), szExePath);
}

Napi::Value ModuleEntry::getModBaseAddr(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), reinterpret_cast<uintptr_t>(modBaseAddr));
}


Napi::Value ModuleEntry::getModBaseSize(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), modBaseSize);
}

Napi::Value ModuleEntry::getTh32ProcessID(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), th32ProcessID);
}

Napi::Value ModuleEntry::getSzModule(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), szModule);
}
      

ModuleEntry::~ModuleEntry() {
  //todo
}
