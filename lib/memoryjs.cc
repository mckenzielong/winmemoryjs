#include <node.h>
#include <node_buffer.h>
#include <windows.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include "module.h"
#include "process.h"
#include "memoryjs.h"
#include "memory.h"
#include "pattern.h"
#include "functions.h"
#include "debugger.h"
#include "process_entry.h"
#include "promise_async.h"
#include <napi.h>

#pragma comment(lib, "psapi.lib")

using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Context;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Number;
using v8::Value;
using v8::Array;
using v8::Local;
using v8::Boolean;

// module Module;
memory Memory;
pattern Pattern;
// functions Functions;

struct Vector3 {
  float x, y, z;
};

struct Vector4 {
  float w, x, y, z;
};

void memoryjs::throwError(char* error, Isolate* isolate) {
  isolate->ThrowException(
    Exception::TypeError(String::NewFromUtf8(isolate, error, v8::NewStringType::kNormal).ToLocalChecked())
  );
  return;
}

void openProcess(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 1 && args.Length() != 2) {
    memoryjs::throwError("requires 1 argument, or 2 arguments if a callback is being used", isolate);
    return;
  }

  if (!args[0]->IsString() && !args[0]->IsNumber()) {
    memoryjs::throwError("first argument must be a string or a number", isolate);
    return;
  }

  if (args.Length() == 2 && !args[1]->IsFunction()) {
    memoryjs::throwError("second argument must be a function", isolate);
    return;
  }

  // Define error message that may be set by the function that opens the process
  char* errorMessage = "";

  Process::Pair pair;

  if (args[0]->IsString()) {
    v8::String::Utf8Value processName(isolate, args[0]);  
    pair = Process::openProcess((char*) *(processName), &errorMessage);

    // In case it failed to open, let's keep retrying
    // while(!strcmp(process.szExeFile, "")) {
    //   process = Process.openProcess((char*) *(processName), &errorMessage);
    // };
  }

  if (args[0]->IsNumber()) {
    pair = Process::openProcess((args[0]->Uint32Value(ctx).FromJust()), &errorMessage);

    // In case it failed to open, let's keep retrying
    // while(!strcmp(process.szExeFile, "")) {
    //   process = Process.openProcess(args[0]->Uint32Value(), &errorMessage);
    // };
  }

  // If an error message was returned from the function that opens the process, throw the error.
  // Only throw an error if there is no callback (if there's a callback, the error is passed there).
  if (strcmp(errorMessage, "") && args.Length() != 2) {
    memoryjs::throwError(errorMessage, isolate);
    return;
  }

  // Create a v8 Object (JSON) to store the process information
  Local<Object> processInfo = Object::New(isolate);

  processInfo->Set(ctx, String::NewFromUtf8(isolate, "dwSize", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (int)pair.process.dwSize));
  processInfo->Set(ctx, String::NewFromUtf8(isolate, "th32ProcessID", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (int)pair.process.th32ProcessID));
  processInfo->Set(ctx, String::NewFromUtf8(isolate, "cntThreads", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (int)pair.process.cntThreads));
  processInfo->Set(ctx, String::NewFromUtf8(isolate, "th32ParentProcessID", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (int)pair.process.th32ParentProcessID));
  processInfo->Set(ctx, String::NewFromUtf8(isolate, "pcPriClassBase", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (int)pair.process.pcPriClassBase));
  processInfo->Set(ctx, String::NewFromUtf8(isolate, "szExeFile", v8::NewStringType::kNormal).ToLocalChecked(), 
    String::NewFromUtf8(isolate, (char*)pair.process.szExeFile, v8::NewStringType::kNormal).ToLocalChecked());
  processInfo->Set(ctx, String::NewFromUtf8(isolate, "handle", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (intptr_t)pair.handle));

  DWORD64 base = module::getBaseAddress((char *)pair.process.szExeFile, pair.process.th32ProcessID);
  processInfo->Set(ctx, 
    String::NewFromUtf8(isolate, "modBaseAddr", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (uintptr_t)base));

  // openProcess can either take one argument or can take
  // two arguments for asychronous use (second argument is the callback)
  if (args.Length() == 2) {
    // Callback to let the user handle with the information
    Local<Function> callback = Local<Function>::Cast(args[1]);
    const unsigned argc = 2;
    Local<Value> argv[argc] = { 
      String::NewFromUtf8(isolate, errorMessage, v8::NewStringType::kNormal).ToLocalChecked(), 
      processInfo 
    };
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    // return JSON
    args.GetReturnValue().Set(processInfo);
  }
}

void closeProcess(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 1) {
    memoryjs::throwError("requires 1 argument", isolate);
    return;
  }

  if (!args[0]->IsNumber()) {
    memoryjs::throwError("first argument must be a number", isolate);
    return;
  }

  Process::closeProcess(reinterpret_cast<HANDLE>(static_cast<size_t>(args[0]->IntegerValue(ctx).FromJust())));
}

Napi::Value getProcesses(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  //arugment checking
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

Napi::Value getProcessesSync(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  //arugment checking
  if (info.Length() > 0) {
    throw Napi::Error::New(env, "sync call takes no arguments");
  }

  Napi::Value processes = Process::getProcesses(env);
  return processes;    
}
 
void getModules(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 1 && args.Length() != 2) {
    memoryjs::throwError("requires 1 argument, or 2 arguments if a callback is being used", isolate);
    return;
  }

  if (!args[0]->IsNumber()) {
    memoryjs::throwError("first argument must be a number", isolate);
    return;
  }

  if (args.Length() == 2 && !args[1]->IsFunction()) {
    memoryjs::throwError("first argument must be a number, second argument must be a function", isolate);
    return;
  }

  // Define error message that may be set by the function that gets the modules
  char* errorMessage = "";

  std::vector<MODULEENTRY32> moduleEntries = module::getModules(args[0]->Int32Value(ctx).FromJust(), &errorMessage);

  // If an error message was returned from the function getting the modules, throw the error.
  // Only throw an error if there is no callback (if there's a callback, the error is passed there).
  if (strcmp(errorMessage, "") && args.Length() != 2) {
    memoryjs::throwError(errorMessage, isolate);
    return;
  }

  // Creates v8 array with the size being that of the moduleEntries vector
  // modules is an array of JavaScript objects
  Local<Array> modules = Array::New(isolate, moduleEntries.size());

  // Loop over all modules found
  for (std::vector<MODULEENTRY32>::size_type i = 0; i != moduleEntries.size(); i++) {
    //  Create a v8 object to store the current module's information
    Local<Object> module = Object::New(isolate);

    module->Set(ctx, String::NewFromUtf8(isolate, "modBaseAddr", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (uintptr_t)moduleEntries[i].modBaseAddr));
    module->Set(ctx, String::NewFromUtf8(isolate, "modBaseSize", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (int)moduleEntries[i].modBaseSize));
    module->Set(ctx, String::NewFromUtf8(isolate, "szExePath", v8::NewStringType::kNormal).ToLocalChecked(), 
      String::NewFromUtf8(isolate, (char *)moduleEntries[i].szExePath, v8::NewStringType::kNormal).ToLocalChecked());
    module->Set(ctx, String::NewFromUtf8(isolate, "szModule", v8::NewStringType::kNormal).ToLocalChecked(), 
      String::NewFromUtf8(isolate, (char *)moduleEntries[i].szModule, v8::NewStringType::kNormal).ToLocalChecked());
    module->Set(ctx, String::NewFromUtf8(isolate, "th32ModuleID", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (int)moduleEntries[i].th32ProcessID));

    // Push the object to the array
    modules->Set(ctx, i, module);
  }

  // getModules can either take one argument or two arguments
  // one/two arguments is for asychronous use (the callback)
  if (args.Length() == 2) {
    // Callback to let the user handle with the information
    Local<Function> callback = Local<Function>::Cast(args[1]);
    const unsigned argc = 2;
    Local<Value> argv[argc] = { String::NewFromUtf8(isolate, errorMessage, v8::NewStringType::kNormal).ToLocalChecked(), modules };
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    // return JSON
    args.GetReturnValue().Set(modules);
  }
}

void findModule(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 1 && args.Length() != 2 && args.Length() != 3) {
    memoryjs::throwError("requires 1 argument, 2 arguments, or 3 arguments if a callback is being used", isolate);
    return;
  }

  if (!args[0]->IsString() && !args[1]->IsNumber()) {
    memoryjs::throwError("first argument must be a string, second argument must be a number", isolate);
    return;
  }

  if (args.Length() == 3 && !args[2]->IsFunction()) {
    memoryjs::throwError("third argument must be a function", isolate);
    return;
  }
	
  v8::String::Utf8Value moduleName(isolate, args[0]);
	
  // Define error message that may be set by the function that gets the modules
  char* errorMessage = "";

  MODULEENTRY32 module = module::findModule((char*) *(moduleName), args[1]->Int32Value(ctx).FromJust(), &errorMessage);

  // If an error message was returned from the function getting the module, throw the error.
  // Only throw an error if there is no callback (if there's a callback, the error is passed there).
  if (strcmp(errorMessage, "") && args.Length() != 3) {
    memoryjs::throwError(errorMessage, isolate);
    return;
  }

  // In case it failed to open, let's keep retrying
  while (!strcmp((char *)module.szExePath, "")) {
    module = module::findModule((char*) *(moduleName), args[1]->Int32Value(ctx).FromJust(), &errorMessage);
  };

  // Create a v8 Object (JSON) to store the process information
  Local<Object> moduleInfo = Object::New(isolate);

  moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "modBaseAddr", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (uintptr_t)module.modBaseAddr));
  moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "modBaseSize", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (int)module.modBaseSize));
  moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "szExePath", v8::NewStringType::kNormal).ToLocalChecked(), 
    String::NewFromUtf8(isolate, (char *)module.szExePath, v8::NewStringType::kNormal).ToLocalChecked());
  moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "szModule", v8::NewStringType::kNormal).ToLocalChecked(), 
    String::NewFromUtf8(isolate, (char *)module.szModule, v8::NewStringType::kNormal).ToLocalChecked());
  moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "th32ProcessID", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (int)module.th32ProcessID));
  moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "hModule", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (uintptr_t)module.hModule));

  // findModule can either take one or two arguments,
  // three arguments for asychronous use (third argument is the callback)
  if (args.Length() == 3) {
    // Callback to let the user handle with the information
    Local<Function> callback = Local<Function>::Cast(args[2]);
    const unsigned argc = 2;
    Local<Value> argv[argc] = { 
      String::NewFromUtf8(isolate, errorMessage, v8::NewStringType::kNormal).ToLocalChecked(), 
      moduleInfo 
    };
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    // return JSON
    args.GetReturnValue().Set(moduleInfo);
  }
}

void readMemory(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 3 && args.Length() != 4) {
    memoryjs::throwError("requires 3 arguments, or 4 arguments if a callback is being used", isolate);
    return;
  }

  if (!args[0]->IsNumber() && !args[1]->IsNumber() && !args[2]->IsString()) {
    memoryjs::throwError("first and second argument must be a number, third argument must be a string", isolate);
    return;
  }

  if (args.Length() == 4 && !args[3]->IsFunction()) {
    memoryjs::throwError("fourth argument must be a function", isolate);
    return;
  }

  v8::String::Utf8Value dataTypeArg(isolate, args[2]);
  char* dataType = (char*) *(dataTypeArg);

  // Set callback variables in the case the a callback parameter has been passed
  Local<Function> callback = Local<Function>::Cast(args[3]);
  const unsigned argc = 2;
  Local<Value> argv[argc];

  // Define the error message that will be set if no data type is recognised
  argv[0] = String::NewFromUtf8(isolate, "", v8::NewStringType::kNormal).ToLocalChecked();

  HANDLE handle = reinterpret_cast<HANDLE>(static_cast<size_t>(args[0]->IntegerValue(ctx).FromJust()));
  DWORD64 address = args[1]->IntegerValue(ctx).FromJust();

  if (!strcmp(dataType, "byte")) {

    unsigned char result = Memory.readMemory<unsigned char>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "int")) {

    int result = Memory.readMemory<int>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "int32")) {

    int32_t result = Memory.readMemory<int32_t>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "uint32")) {

    uint32_t result = Memory.readMemory<uint32_t>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "int64")) {

    int64_t result = Memory.readMemory<int64_t>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "uint64")) {

    uint64_t result = Memory.readMemory<uint64_t>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "dword")) {

    DWORD result = Memory.readMemory<DWORD>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "short")) {

    short result = Memory.readMemory<short>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "long")) {

    long result = Memory.readMemory<long>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "float")) {

    float result = Memory.readMemory<float>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "double")) {
		
    double result = Memory.readMemory<double>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "ptr") || !strcmp(dataType, "pointer")) {

    intptr_t result = Memory.readMemory<intptr_t>(handle, address);
    if (args.Length() == 4) argv[1] = Number::New(isolate, result);
    else args.GetReturnValue().Set(Number::New(isolate, result));

  } else if (!strcmp(dataType, "bool") || !strcmp(dataType, "boolean")) {

    bool result = Memory.readMemory<bool>(handle, address);
    if (args.Length() == 4) argv[1] = Boolean::New(isolate, result);
    else args.GetReturnValue().Set(Boolean::New(isolate, result));

  } else if (!strcmp(dataType, "string") || !strcmp(dataType, "str")) {

    std::vector<char> chars;
    int offset = 0x0;
    while (true) {
      char c = Memory.readChar(handle, address + offset);
      chars.push_back(c);

      // break at 1 million chars
      if (offset == (sizeof(char) * 1000000)) {
        chars.clear();
        break;
      }

      // break at terminator (end of string)
      if (c == '\0') {
        break;
      }

	  // go to next char
      offset += sizeof(char);
    }

    if (chars.size() == 0) {
    
      if (args.Length() == 4) argv[0] = String::NewFromUtf8(isolate, "unable to read string (no null-terminator found after 1 million chars)", v8::NewStringType::kNormal).ToLocalChecked();
      else return memoryjs::throwError("unable to read string (no null-terminator found after 1 million chars)", isolate);
    
    } else {
      // vector -> string
      std::string str(chars.begin(), chars.end());

      if (args.Length() == 4) argv[1] = String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
      else args.GetReturnValue().Set(String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
    
    }

  } else if (!strcmp(dataType, "vector3") || !strcmp(dataType, "vec3")) {

    Vector3 result = Memory.readMemory<Vector3>(handle, address);
    Local<Object> moduleInfo = Object::New(isolate);
    moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "x", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, result.x));
    moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "y", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, result.y));
    moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "z", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, result.z));

    if (args.Length() == 4) argv[1] = moduleInfo;
    else args.GetReturnValue().Set(moduleInfo);

  } else if (!strcmp(dataType, "vector4") || !strcmp(dataType, "vec4")) {
    
    Vector4 result = Memory.readMemory<Vector4>(handle, address);
    Local<Object> moduleInfo = Object::New(isolate);
    moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "w", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, result.w));
    moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "x", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, result.x));
    moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "y", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, result.y));
    moduleInfo->Set(ctx, String::NewFromUtf8(isolate, "z", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, result.z));

    if (args.Length() == 4) argv[1] = moduleInfo;
    else args.GetReturnValue().Set(moduleInfo);

  } else {

    if (args.Length() == 4) argv[0] = String::NewFromUtf8(isolate, "unexpected data type", v8::NewStringType::kNormal).ToLocalChecked();
    else return memoryjs::throwError("unexpected data type", isolate);

  }

  if (args.Length() == 4) callback->Call(ctx, Null(isolate), argc, argv);
}

void readBuffer(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 3 && args.Length() != 4) {
    memoryjs::throwError("requires 3 arguments, or 4 arguments if a callback is being used", isolate);
    return;
  }

  if (!args[0]->IsNumber() && !args[1]->IsNumber() && !args[2]->IsNumber()) {
    memoryjs::throwError("first, second and third arguments must be a number", isolate);
    return;
  }

  if (args.Length() == 4 && !args[3]->IsFunction()) {
    memoryjs::throwError("fourth argument must be a function", isolate);
    return;
  }

  // Set callback variables in the case the a callback parameter has been passed
  Local<Function> callback = Local<Function>::Cast(args[3]);
  const unsigned argc = 2;
  Local<Value> argv[argc];

  // Define the error message that will be set if no data type is recognised
  argv[0] = String::NewFromUtf8(isolate, "", v8::NewStringType::kNormal).ToLocalChecked();

  HANDLE handle = (HANDLE)args[0]->IntegerValue(ctx).FromJust();
  DWORD64 address = args[1]->IntegerValue(ctx).FromJust();
  SIZE_T size = args[2]->IntegerValue(ctx).FromJust();
  char* data = Memory.readBuffer(handle, address, size);

  auto buffer = node::Buffer::New(isolate, data, size).ToLocalChecked();

  if (args.Length() == 4) {
    argv[1] = buffer;
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    args.GetReturnValue().Set(buffer);
  }
}

void writeMemory(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 4) {
    memoryjs::throwError("requires 4 arguments", isolate);
    return;
  }

  if (!args[0]->IsNumber() && !args[1]->IsNumber() && !args[3]->IsString()) {
    memoryjs::throwError("first and second argument must be a number, third argument must be a string", isolate);
    return;
  }

  v8::String::Utf8Value dataTypeArg(isolate, args[3]);
  char* dataType = (char*)*(dataTypeArg);

  HANDLE handle = (HANDLE)args[0]->IntegerValue(ctx).FromJust();
  DWORD64 address = args[1]->IntegerValue(ctx).FromJust();

  if (!strcmp(dataType, "byte")) {
  
    Memory.writeMemory<unsigned char>(handle, address, args[2]->Uint32Value(ctx).FromJust());
  
  } else if (!strcmp(dataType, "int")) {

    Memory.writeMemory<int>(handle, address, args[2]->NumberValue(ctx).FromJust());

  } else if (!strcmp(dataType, "int32")) {

    Memory.writeMemory<int32_t>(handle, address, args[2]->Int32Value(ctx).FromJust());

  } else if (!strcmp(dataType, "uint32")) {

    Memory.writeMemory<uint32_t>(handle, address, args[2]->Uint32Value(ctx).FromJust());

  } else if (!strcmp(dataType, "int64")) {

    Memory.writeMemory<int64_t>(handle, address, args[2]->IntegerValue(ctx).FromJust());

  } else if (!strcmp(dataType, "uint64")) {

    Memory.writeMemory<uint64_t>(handle, address, args[2]->NumberValue(ctx).FromJust());

  } else if (!strcmp(dataType, "dword")) {

    Memory.writeMemory<DWORD>(handle, address, args[2]->NumberValue(ctx).FromJust());

  } else if (!strcmp(dataType, "short")) {

    Memory.writeMemory<short>(handle, address, args[2]->NumberValue(ctx).FromJust());

  } else if (!strcmp(dataType, "long")) {

    Memory.writeMemory<long>(handle, address, args[2]->NumberValue(ctx).FromJust());

  } else if (!strcmp(dataType, "float")) {

    Memory.writeMemory<float>(handle, address, args[2]->NumberValue(ctx).FromJust());

  } else if (!strcmp(dataType, "double")) {

    Memory.writeMemory<double>(handle, address, args[2]->NumberValue(ctx).FromJust());

  } else if (!strcmp(dataType, "bool") || !strcmp(dataType, "boolean")) {

    Memory.writeMemory<bool>(handle, address, args[2]->BooleanValue(isolate));

  } else if (!strcmp(dataType, "string") || !strcmp(dataType, "str")) {

    v8::String::Utf8Value valueParam(isolate, args[2]->ToString(ctx).ToLocalChecked());
    
    // Write String, Method 1
    //Memory.writeMemory<std::string>(handle, address, std::string(*valueParam));

    // Write String, Method 2
    Memory.writeMemory(handle, address, *valueParam, valueParam.length());
    
  } else if (!strcmp(dataType, "vector3") || !strcmp(dataType, "vec3")) {

    Local<Object> value = Local<Object>::Cast(args[2]);
    Vector3 vector = {
      static_cast<float>(value->Get(ctx, String::NewFromUtf8(isolate, "x", v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked()->NumberValue(ctx).FromJust()),
      static_cast<float>(value->Get(ctx, String::NewFromUtf8(isolate, "y", v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked()->NumberValue(ctx).FromJust()),
      static_cast<float>(value->Get(ctx, String::NewFromUtf8(isolate, "z", v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked()->NumberValue(ctx).FromJust())
    };
    Memory.writeMemory<Vector3>(handle, address, vector);

  } else if (!strcmp(dataType, "vector4") || !strcmp(dataType, "vec4")) {

    Local<Object> value = Local<Object>::Cast(args[2]);
    Vector4 vector = {
      static_cast<float>(value->Get(ctx, String::NewFromUtf8(isolate, "w", v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked()->NumberValue(ctx).FromJust()),
      static_cast<float>(value->Get(ctx, String::NewFromUtf8(isolate, "x", v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked()->NumberValue(ctx).FromJust()),
      static_cast<float>(value->Get(ctx, String::NewFromUtf8(isolate, "y", v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked()->NumberValue(ctx).FromJust()),
      static_cast<float>(value->Get(ctx, String::NewFromUtf8(isolate, "z", v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked()->NumberValue(ctx).FromJust())
    };
    Memory.writeMemory<Vector4>(handle, address, vector);

  } else {
    
    memoryjs::throwError("unexpected data type", isolate);

  }
}

void writeBuffer(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 3) {
    memoryjs::throwError("required 3 arguments", isolate);
    return;
  }

  if (!args[0]->IsNumber() && !args[1]->IsNumber()) {
    memoryjs::throwError("first and second argument must be a number", isolate);
    return;
  }

  HANDLE handle = (HANDLE)args[0]->IntegerValue(ctx).FromJust();
  DWORD64 address = args[1]->IntegerValue(ctx).FromJust();
  SIZE_T length = node::Buffer::Length(args[2]);
  char* data = node::Buffer::Data(args[2]);
  Memory.writeMemory<char*>(handle, address, data, length);
}

void findPattern(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  // if (args.Length() != 5 && args.Length() != 6) {
  //   memoryjs::throwError("requires 5 arguments, or 6 arguments if a callback is being used", isolate);
  //   return;
  // }

  // if (!args[0]->IsNumber() && !args[1]->IsString() && !args[2]->IsNumber() && !args[3]->IsNumber() && !args[4]->IsNumber()) {
  //   memoryjs::throwError("first argument must be a number, the remaining arguments must be numbers apart from the callback", isolate);
  //   return;
  // }

  // if (args.Length() == 6 && !args[5]->IsFunction()) {
  //   memoryjs::throwError("sixth argument must be a function", isolate);
  //   return;
  // }

  // Address of findPattern result
  uintptr_t address = -1;

  // Define error message that may be set by the function that gets the modules
  char* errorMessage = "";

  HANDLE handle = (HANDLE)args[0]->IntegerValue(ctx).FromJust();

  std::vector<MODULEENTRY32> moduleEntries = module::getModules(GetProcessId(handle), &errorMessage);

  // If an error message was returned from the function getting the modules, throw the error.
  // Only throw an error if there is no callback (if there's a callback, the error is passed there).
  if (strcmp(errorMessage, "") && args.Length() != 7) {
    memoryjs::throwError(errorMessage, isolate);
    return;
  }

  for (std::vector<MODULEENTRY32>::size_type i = 0; i != moduleEntries.size(); i++) {
    v8::String::Utf8Value moduleName(isolate, args[1]);

    if (!strcmp((char *)moduleEntries[i].szModule, std::string(*moduleName).c_str())) {
      v8::String::Utf8Value signature(isolate, args[2]->ToString(ctx).ToLocalChecked());

      // const char* pattern = std::string(*signature).c_str();
      short sigType = args[3]->Uint32Value(ctx).FromJust();
      uint32_t patternOffset = args[4]->Uint32Value(ctx).FromJust();
      uint32_t addressOffset = args[5]->Uint32Value(ctx).FromJust();

      address = Pattern.findPattern(handle, moduleEntries[i], std::string(*signature).c_str(), sigType, patternOffset, addressOffset);
      break;
    }
  }

  // If no error was set by getModules and the address is still the value we set it as, it probably means we couldn't find the module
  if (strcmp(errorMessage, "") && address == -1) errorMessage = "unable to find module";

  // If no error was set by getModules and the address is -2 this means there was no match to the pattern
  if (strcmp(errorMessage, "") && address == -2) errorMessage = "no match found";

  // findPattern can be asynchronous
  if (args.Length() == 7) {
    // Callback to let the user handle with the information
    Local<Function> callback = Local<Function>::Cast(args[6]);
    const unsigned argc = 2;
    Local<Value> argv[argc] = { 
      String::NewFromUtf8(isolate, errorMessage, v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, address) 
    };
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    // return JSON
    args.GetReturnValue().Set(Number::New(isolate, address));
  }
}

void callFunction(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 4 && args.Length() != 5) {
    memoryjs::throwError("requires 4 arguments, 5 with callback", isolate);
    return;
  }

  if (!args[0]->IsNumber() && !args[1]->IsObject() && !args[2]->IsNumber() && !args[3]->IsNumber()) {
    memoryjs::throwError("invalid arguments", isolate);
    return;
  }

  // TODO: temp (?) solution to forcing variables onto the heap
  // to ensure consistent addresses. copy everything to a vector, and use the
  // vector's instances of the variables as the addresses being passed to `functions.call()`.
  // Another solution: do `int x = new int(4)` and then use `&x` for the address
  std::vector<LPVOID> heap;

  std::vector<Arg> parsedArgs;
  Local<Array> arguments = Local<Array>::Cast(args[1]);
  for (unsigned int i = 0; i < arguments->Length(); i++) {
    Local<Object> argument = Local<Object>::Cast(arguments->Get(ctx, i).ToLocalChecked());

    Type type = (Type) argument->Get(ctx, String::NewFromUtf8(isolate, "type", v8::NewStringType::kNormal).ToLocalChecked())
      .ToLocalChecked()->Uint32Value(ctx).FromJust();
    
    if (type == T_STRING) {
      Local<Value> data = argument->Get(ctx, String::NewFromUtf8(isolate, "value", v8::NewStringType::kNormal).ToLocalChecked())
        .ToLocalChecked();
      v8::String::Utf8Value stringValueUtf(isolate, data->ToString(ctx).ToLocalChecked());
      std::string stringValue = std::string(*stringValueUtf);
      parsedArgs.push_back({ type, &stringValue });
    }

    if (type == T_INT) {
      int data = argument->Get(ctx, String::NewFromUtf8(isolate, "value", v8::NewStringType::kNormal).ToLocalChecked())
        .ToLocalChecked()->NumberValue(ctx).FromJust();

      // As we only pass the addresses of the variable to the `call` function and not a copy
      // of the variable itself, we need to ensure that the variable stays alive and in a unique
      // memory location until the `call` function has been executed. So manually allocate memory,
      // track it, and then free it once the function has been called.
      // TODO: find a better solution?
      int* memory = (int*) malloc(sizeof(int));
      *memory = data;
      heap.push_back(memory);

      parsedArgs.push_back({ type, memory });
    }

    if (type == T_FLOAT) {
      float data = argument->Get(ctx, String::NewFromUtf8(isolate, "value", v8::NewStringType::kNormal).ToLocalChecked())
        .ToLocalChecked()->NumberValue(ctx).FromJust();

      float* memory = (float*) malloc(sizeof(float));
      *memory = data;
      heap.push_back(memory);

      parsedArgs.push_back({ type, memory });
    }
  }

  HANDLE handle = (HANDLE)args[0]->IntegerValue(ctx).FromJust();
  Type returnType = (Type) args[2]->Uint32Value(ctx).FromJust();
  DWORD64 address = args[3]->NumberValue(ctx).FromJust();

  char* errorMessage = "";
  Call data = functions::call<int>(handle, parsedArgs, returnType, address, &errorMessage);

  // Free all the memory we allocated
  for (auto &memory : heap) {
    free(memory);
  }

  heap.clear();

  Local<Object> info = Object::New(isolate);
  Local<String> keyString = String::NewFromUtf8(isolate, "returnValue", v8::NewStringType::kNormal).ToLocalChecked();
  
  if (returnType == T_STRING) {
    info->Set(ctx, keyString, String::NewFromUtf8(isolate, data.returnString.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
  }
  
  if (returnType == T_CHAR) {
    info->Set(ctx, keyString, Number::New(isolate, *reinterpret_cast<char *>(&data.returnValue)));
  }

  if (returnType == T_BOOL) {
    info->Set(ctx, keyString, Number::New(isolate, *reinterpret_cast<bool *>(&data.returnValue)));
  }

  if (returnType == T_INT) {
    info->Set(ctx, keyString, Number::New(isolate, *reinterpret_cast<int *>(&data.returnValue)));
  }

  if (returnType == T_FLOAT) {
    float value = *reinterpret_cast<float *>(&data.returnValue);
    info->Set(ctx, keyString, Number::New(isolate, value));
  }

  if (returnType == T_DOUBLE) {
    double value = *reinterpret_cast<double *>(&data.returnValue);
    info->Set(ctx, keyString, Number::New(isolate, value));
  }

  info->Set(ctx, String::NewFromUtf8(isolate, "exitCode", v8::NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, data.exitCode));

  if (args.Length() == 5) {
    // Callback to let the user handle with the information
    Local<Function> callback = Local<Function>::Cast(args[2]);
    const unsigned argc = 2;
    Local<Value> argv[argc] = { String::NewFromUtf8(isolate, errorMessage, v8::NewStringType::kNormal).ToLocalChecked(), info };
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    // return JSON
    args.GetReturnValue().Set(info);
  }
  
}

void virtualProtectEx(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 4 && args.Length() != 5) {
    memoryjs::throwError("requires 4 arguments, 5 with callback", isolate);
    return;
  }

  if (!args[0]->IsNumber() && !args[1]->IsNumber() && !args[2]->IsNumber()) {
    memoryjs::throwError("All arguments should be numbers.", isolate);
    return;
  }

  if (args.Length() == 5 && !args[4]->IsFunction()) {
    memoryjs::throwError("callback needs to be a function", isolate);
    return;
  }

  DWORD result;
  HANDLE handle = (HANDLE)args[0]->IntegerValue(ctx).FromJust();
  DWORD64 address = args[1]->IntegerValue(ctx).FromJust();
  SIZE_T size = args[2]->IntegerValue(ctx).FromJust();
  DWORD protection = args[3]->Uint32Value(ctx).FromJust();

  bool success = VirtualProtectEx(handle, (LPVOID) address, size, protection, &result);

  char* errorMessage = "";

  if (success == 0) {
    errorMessage = "an error occurred calling VirtualProtectEx";
    // errorMessage = GetLastErrorToString().c_str();
  }

  // If there is an error and there is no callback, throw the error
  if (strcmp(errorMessage, "") && args.Length() != 5) {
    memoryjs::throwError(errorMessage, isolate);
    return;
  }

  if (args.Length() == 5) {
    // Callback to let the user handle with the information
    Local<Function> callback = Local<Function>::Cast(args[5]);
    const unsigned argc = 2;
    Local<Value> argv[argc] = {
      String::NewFromUtf8(isolate, errorMessage, v8::NewStringType::kNormal).ToLocalChecked(),
      Number::New(isolate, result)
    };
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    args.GetReturnValue().Set(Number::New(isolate, result));
  }
}

void getRegions(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 1 && args.Length() != 2) {
    memoryjs::throwError("requires 1 argument, 2 with callback", isolate);
    return;
  }

  if (!args[0]->IsNumber()) {
    memoryjs::throwError("invalid arguments: first argument must be a number", isolate);
    return;
  }

  if (args.Length() == 2 && !args[1]->IsFunction()) {
    memoryjs::throwError("callback needs to be a function", isolate);
    return;
  }

  HANDLE handle = (HANDLE)args[0]->IntegerValue(ctx).FromJust();
  std::vector<MEMORY_BASIC_INFORMATION> regions = Memory.getRegions(handle);

  Local<Array> regionsArray = Array::New(isolate, regions.size());

  for (std::vector<MEMORY_BASIC_INFORMATION>::size_type i = 0; i != regions.size(); i++) {
    Local<Object> region = Object::New(isolate);

    region->Set(ctx, String::NewFromUtf8(isolate, "BaseAddress", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD64) regions[i].BaseAddress));
    region->Set(ctx, String::NewFromUtf8(isolate, "AllocationBase", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD64) regions[i].AllocationBase));
    region->Set(ctx, String::NewFromUtf8(isolate, "AllocationProtect", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD) regions[i].AllocationProtect));
    region->Set(ctx, String::NewFromUtf8(isolate, "RegionSize", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (SIZE_T) regions[i].RegionSize));
    region->Set(ctx, String::NewFromUtf8(isolate, "State", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD) regions[i].State));
    region->Set(ctx, String::NewFromUtf8(isolate, "Protect", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD) regions[i].Protect));
    region->Set(ctx, String::NewFromUtf8(isolate, "Type", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD) regions[i].Type));

    char moduleName[MAX_PATH];
    DWORD size = GetModuleFileNameExA(handle, (HINSTANCE)regions[i].AllocationBase, moduleName, MAX_PATH);

    if (size != 0) {
      region->Set(ctx, String::NewFromUtf8(isolate, "szExeFile", v8::NewStringType::kNormal).ToLocalChecked(), 
        String::NewFromUtf8(isolate, moduleName, v8::NewStringType::kNormal).ToLocalChecked());
    }


    regionsArray->Set(ctx, i, region);
  }

  if (args.Length() == 2) {
    // Callback to let the user handle with the information
    Local<Function> callback = Local<Function>::Cast(args[1]);
    const unsigned argc = 2;
    Local<Value> argv[argc] = { String::NewFromUtf8(isolate, "", v8::NewStringType::kNormal).ToLocalChecked(), regionsArray };
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    // return JSON
    args.GetReturnValue().Set(regionsArray);
  }
}

void virtualQueryEx(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 2 && args.Length() != 3) {
    memoryjs::throwError("requires 2 arguments, 3 with callback", isolate);
    return;
  }

  if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
    memoryjs::throwError("first and second argument need to be a number", isolate);
    return;
  }

  if (args.Length() == 3 && !args[2]->IsFunction()) {
    memoryjs::throwError("callback needs to be a function", isolate);
    return;
  }

  HANDLE handle = (HANDLE)args[0]->IntegerValue(ctx).FromJust();
  DWORD64 address = args[1]->IntegerValue(ctx).FromJust();

  MEMORY_BASIC_INFORMATION information;
  SIZE_T result = VirtualQueryEx(handle, (LPVOID)address, &information, sizeof(information));

  char* errorMessage = "";

  if (result == 0 || result != sizeof(information)) {
    errorMessage = "an error occurred calling VirtualQueryEx";
    // errorMessage = GetLastErrorToString().c_str();
  }

  // If there is an error and there is no callback, throw the error
  if (strcmp(errorMessage, "") && args.Length() != 3) {
    memoryjs::throwError(errorMessage, isolate);
    return;
  }

  Local<Object> region = Object::New(isolate);

  region->Set(ctx, String::NewFromUtf8(isolate, "BaseAddress", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (DWORD64) information.BaseAddress));
  region->Set(ctx, String::NewFromUtf8(isolate, "AllocationBase", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (DWORD64) information.AllocationBase));
  region->Set(ctx, String::NewFromUtf8(isolate, "AllocationProtect", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (DWORD) information.AllocationProtect));
  region->Set(ctx, String::NewFromUtf8(isolate, "RegionSize", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (SIZE_T) information.RegionSize));
  region->Set(ctx, String::NewFromUtf8(isolate, "State", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (DWORD) information.State));
  region->Set(ctx, String::NewFromUtf8(isolate, "Protect", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (DWORD) information.Protect));
  region->Set(ctx, String::NewFromUtf8(isolate, "Type", v8::NewStringType::kNormal).ToLocalChecked(), 
    Number::New(isolate, (DWORD) information.Type));

  if (args.Length() == 3) {
    // Callback to let the user handle with the information
    Local<Function> callback = Local<Function>::Cast(args[1]);
    const unsigned argc = 2;
    Local<Value> argv[argc] = { String::NewFromUtf8(isolate, "", v8::NewStringType::kNormal).ToLocalChecked(), region };
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    // return JSON
    args.GetReturnValue().Set(region);
  }
}

void virtualAllocEx(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 5 && args.Length() != 6) {
    memoryjs::throwError("requires 5 arguments, 6 with callback", isolate);
    return;
  }

  if (!args[0]->IsNumber() || !args[2]->IsNumber() || !args[3]->IsNumber() || !args[4]->IsNumber()) {
    memoryjs::throwError("invalid arguments: arguments 0, 2, 3 and 4 need to be numbers", isolate);
    return;
  }

  if (args.Length() == 6 && !args[5]->IsFunction()) {
    memoryjs::throwError("callback needs to be a function", isolate);
    return;
  }

  HANDLE handle = (HANDLE)args[0]->IntegerValue(ctx).FromJust();
  SIZE_T size = args[2]->IntegerValue(ctx).FromJust();
  DWORD allocationType = args[3]->Uint32Value(ctx).FromJust();
  DWORD protection = args[4]->Uint32Value(ctx).FromJust();
  LPVOID address;

  // Means in the JavaScript space `null` was passed through.
  if (args[1] == Null(isolate)) {
    address = NULL;
  } else {
    address = (LPVOID) args[1]->IntegerValue(ctx).FromJust();
  }

  LPVOID allocatedAddress = VirtualAllocEx(handle, address, size, allocationType, protection);

  char* errorMessage = "";

  // If null, it means an error occurred
  if (allocatedAddress == NULL) {
    errorMessage = "an error occurred calling VirtualAllocEx";
    // errorMessage = GetLastErrorToString().c_str();
  }

  // If there is an error and there is no callback, throw the error
  if (strcmp(errorMessage, "") && args.Length() != 6) {
    memoryjs::throwError(errorMessage, isolate);
    return;
  }

  if (args.Length() == 6) {
    // Callback to let the user handle with the information
    Local<Function> callback = Local<Function>::Cast(args[5]);
    const unsigned argc = 2;
    Local<Value> argv[argc] = {
      String::NewFromUtf8(isolate, errorMessage, v8::NewStringType::kNormal).ToLocalChecked(),
      Number::New(isolate, *reinterpret_cast<int *>(&allocatedAddress))
    };
    callback->Call(ctx, Null(isolate), argc, argv);
  } else {
    args.GetReturnValue().Set(Number::New(isolate, *reinterpret_cast<int *>(&allocatedAddress)));
  }
}

void attachDebugger(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 2) {
    memoryjs::throwError("requires 2 arguments", isolate);
    return;
  }

  if (!args[0]->IsNumber() || !args[1]->IsBoolean()) {
    memoryjs::throwError("first argument needs to be a number, second a boolean", isolate);
    return;
  }

  DWORD processId = args[0]->Uint32Value(ctx).FromJust();
  bool killOnExit = args[1]->BooleanValue(isolate);

  bool success = debugger::attach(processId, killOnExit);
  args.GetReturnValue().Set(Boolean::New(isolate, success));
}

void detatchDebugger(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  DWORD processId = args[0]->Uint32Value(isolate->GetCurrentContext()).FromJust();

  if (args.Length() != 1) {
    memoryjs::throwError("requires only 1 argument", isolate);
    return;
  }

  if (!args[0]->IsNumber()) {
    memoryjs::throwError("only argument needs to be a number", isolate);
    return;
  }
  
  bool success = debugger::detatch(processId);
  args.GetReturnValue().Set(Boolean::New(isolate, success));
}

void awaitDebugEvent(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 2) {
    memoryjs::throwError("requires 2 arguments", isolate);
    return;
  }

  if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
    memoryjs::throwError("both arguments need to be a number", isolate);
    return;
  }

  int millisTimeout = args[1]->Uint32Value(ctx).FromJust();

  DebugEvent debugEvent;
  bool success = debugger::awaitDebugEvent(millisTimeout, &debugEvent);

  Register hardwareRegister = static_cast<Register>(args[0]->Uint32Value(ctx).FromJust());

  if (success && debugEvent.hardwareRegister == hardwareRegister) {
    Local<Object> info = Object::New(isolate);

    info->Set(ctx, String::NewFromUtf8(isolate, "processId", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD) debugEvent.processId));
    info->Set(ctx, String::NewFromUtf8(isolate, "threadId", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD) debugEvent.threadId));
    info->Set(ctx, String::NewFromUtf8(isolate, "exceptionCode", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD) debugEvent.exceptionCode));
    info->Set(ctx, String::NewFromUtf8(isolate, "exceptionFlags", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD) debugEvent.exceptionFlags));
    info->Set(ctx, String::NewFromUtf8(isolate, "exceptionAddress", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, (DWORD64) debugEvent.exceptionAddress));
    info->Set(ctx, String::NewFromUtf8(isolate, "hardwareRegister", v8::NewStringType::kNormal).ToLocalChecked(), 
      Number::New(isolate, static_cast<int>(debugEvent.hardwareRegister))); 
  
    args.GetReturnValue().Set(info);
  }

  // If we aren't interested in passing this event back to the JS space,
  // just silently handle it
  if (success && debugEvent.hardwareRegister != hardwareRegister) {
    debugger::handleDebugEvent(debugEvent.processId, debugEvent.threadId);
  }
}

void handleDebugEvent(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 2) {
    memoryjs::throwError("requires 2 arguments", isolate);
    return;
  }

  if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
    memoryjs::throwError("both arguments need to be numbers", isolate);
    return;
  }

  DWORD processId = args[0]->Uint32Value(ctx).FromJust();
  DWORD threadId = args[1]->Uint32Value(ctx).FromJust();

  bool success = debugger::handleDebugEvent(processId, threadId);
  args.GetReturnValue().Set(Boolean::New(isolate, success));
}

void setHardwareBreakpoint(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 5) {
    memoryjs::throwError("requires 5 arguments", isolate);
    return;
  }

  for (int i = 0; i < args.Length(); i++) {
    if (!args[i]->IsNumber()) {
      memoryjs::throwError("all arguments need to be numbers", isolate);
      return;
    }
  }

  DWORD processId = args[0]->Uint32Value(ctx).FromJust();
  DWORD64 address = args[1]->IntegerValue(ctx).FromJust();
  Register hardwareRegister = static_cast<Register>(args[2]->Uint32Value(ctx).FromJust());

  // Execute = 0x0
  // Access = 0x3
  // Writer = 0x1
  int trigger = args[3]->Uint32Value(ctx).FromJust();
  
  int length = args[4]->Uint32Value(ctx).FromJust();

  bool success = debugger::setHardwareBreakpoint(processId, address, hardwareRegister, trigger, length);
  args.GetReturnValue().Set(Boolean::New(isolate, success));
}

void removeHardwareBreakpoint(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> ctx = isolate->GetCurrentContext();

  if (args.Length() != 2) {
    memoryjs::throwError("requires 2 arguments", isolate);
    return;
  }

  if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
    memoryjs::throwError("both arguments need to be numbers", isolate);
    return;
  }

  DWORD processId = args[0]->Uint32Value(ctx).FromJust();
  Register hardwareRegister = static_cast<Register>(args[1]->Uint32Value(ctx).FromJust());

  bool success = debugger::setHardwareBreakpoint(processId, 0, hardwareRegister, 0, 0);
  args.GetReturnValue().Set(Boolean::New(isolate, success));
}

// https://stackoverflow.com/a/17387176
std::string GetLastErrorToString() {
  DWORD errorMessageID = ::GetLastError();
    
  // No error message, return empty string
  if(errorMessageID == 0) {
    return std::string();
  }

  LPSTR messageBuffer = nullptr;

  size_t size = FormatMessageA(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    errorMessageID,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPSTR)&messageBuffer,
    0,
    NULL
  );

  std::string message(messageBuffer, size);

  // Free the buffer
  LocalFree(messageBuffer);
  return message;
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "openProcess", openProcess);
  NODE_SET_METHOD(exports, "closeProcess", closeProcess);
  //NODE_SET_METHOD(exports, "getProcesses", getProcesses);
  NODE_SET_METHOD(exports, "getModules", getModules);
  NODE_SET_METHOD(exports, "findModule", findModule);
  NODE_SET_METHOD(exports, "readMemory", readMemory);
  NODE_SET_METHOD(exports, "readBuffer", readBuffer);
  NODE_SET_METHOD(exports, "writeMemory", writeMemory);
  NODE_SET_METHOD(exports, "writeBuffer", writeBuffer);
  NODE_SET_METHOD(exports, "findPattern", findPattern);
  NODE_SET_METHOD(exports, "virtualProtectEx", virtualProtectEx);
  NODE_SET_METHOD(exports, "callFunction", callFunction);
  NODE_SET_METHOD(exports, "virtualAllocEx", virtualAllocEx);
  NODE_SET_METHOD(exports, "getRegions", getRegions);
  NODE_SET_METHOD(exports, "virtualQueryEx", virtualQueryEx);
  NODE_SET_METHOD(exports, "attachDebugger", attachDebugger);
  NODE_SET_METHOD(exports, "detatchDebugger", detatchDebugger);
  NODE_SET_METHOD(exports, "awaitDebugEvent", awaitDebugEvent);
  NODE_SET_METHOD(exports, "handleDebugEvent", handleDebugEvent);
  NODE_SET_METHOD(exports, "setHardwareBreakpoint", setHardwareBreakpoint);
  NODE_SET_METHOD(exports, "removeHardwareBreakpoint", removeHardwareBreakpoint);
}

//NODE_MODULE(memoryjs, init)

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "getProcessesSync"),
              Napi::Function::New(env, getProcessesSync));
  exports.Set(Napi::String::New(env, "getProcesses"),
              Napi::Function::New(env, getProcesses));

  MemoryAPI::ProcessEntry::Init(env, exports);

  return exports;
}

NODE_API_MODULE(memoryjs, Init)