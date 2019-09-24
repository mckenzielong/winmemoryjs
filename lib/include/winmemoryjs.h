#pragma once
#ifndef WINMEMORYJS_H
#define WINMEMORYJS_H

#include <napi.h>

namespace MemoryAPI {
  Napi::Value getProcesses(const Napi::CallbackInfo& info);
  Napi::Value getProcessesSync(const Napi::CallbackInfo& info);
}

#endif
#pragma once
