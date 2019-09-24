#include "module.h"
#include "process.h"
#include "winmemoryjs.h"
#include "process_entry.h"
#include "module_entry.h"
#include "thread_entry.h"
#include "promise_async.h"

Napi::Value MemoryAPI::getProcesses(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  Napi::Function callback;
  if (info.Length() > 1) {
    throw Napi::Error::New(env, "requires either 0 arguments or 1 argument if a callback is being used");
  } else if (info.Length() == 1 && !info[0].IsFunction()) {
    throw Napi::Error::New(env, "first argument must be a function");
  } else if (info.Length() == 1 && info[0].IsFunction()) {
    callback = info[0].As<Napi::Function>();
  }

  auto deferred = Napi::Promise::Deferred::New(env);
  AsyncHelper::PromiseAsyncWorker<std::vector<PROCESSENTRY32>>* asyncWork;
  if (callback) {
    asyncWork = new AsyncHelper::PromiseAsyncWorker<std::vector<PROCESSENTRY32>>(callback, deferred, Process::getWindowsProcesses, Process::convertProcessEntryArray);
  } else {
    asyncWork = new AsyncHelper::PromiseAsyncWorker<std::vector<PROCESSENTRY32>>(env, deferred, Process::getWindowsProcesses, Process::convertProcessEntryArray);
  }

  asyncWork->Queue();
  return deferred.Promise();    
}

Napi::Value MemoryAPI::getProcessesSync(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  //arugment checking
  if (info.Length() > 0) {
    throw Napi::Error::New(env, "sync call takes no arguments");
  }

  auto winProcs = Process::getWindowsProcesses(env);
  Napi::Value processes = Process::convertProcessEntryArray(env, winProcs);
  return processes;    
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "getProcessesSync"),
              Napi::Function::New(env, MemoryAPI::getProcessesSync));
  exports.Set(Napi::String::New(env, "getProcesses"),
              Napi::Function::New(env, MemoryAPI::getProcesses));

  MemoryAPI::ProcessEntry::Init(env, exports);
  MemoryAPI::ModuleEntry::Init(env, exports);
  MemoryAPI::ThreadEntry::Init(env, exports);

  return exports;
}

NODE_API_MODULE(memoryjs, Init)