#pragma once
#ifndef PROMISE_ASYNC
#define PROMISE_ASYNC

#include <napi.h>

namespace AsyncHelper {
  template <class T>
  class PromiseAsyncWorker : public Napi::AsyncWorker {
    public:    
      PromiseAsyncWorker(const Napi::Env& env, const Napi::Promise::Deferred& promise, T (*onExecute)(Napi::Env), Napi::Value (*onComplete)(Napi::Env, const T&)) : 
        AsyncWorker(env), promise(promise), execute(onExecute), complete(onComplete)  {};
      PromiseAsyncWorker(const Napi::Function& callback, const Napi::Promise::Deferred& promise, T (*onExecute)(Napi::Env), Napi::Value (*onComplete)(Napi::Env, const T&)) :  
        AsyncWorker(callback), promise(promise), execute(onExecute), complete(onComplete)  {};
      ~PromiseAsyncWorker() {};

      void Execute() {
        executeResult = execute(Env());
      }
      
      void OnOK() {
        Napi::HandleScope scope(Env());
        Napi::Value result = complete(Env(), executeResult);
        if (Callback()) {
          Callback().Call({Env().Null(), result});
        }
        promise.Resolve(result);
      }

      void OnError(const Napi::Error& e) {
        Napi::HandleScope scope(Env());
        if (Callback()) {
          Callback().Call({e.Value(), Env().Null()});
        }
        promise.Reject(e.Value());
      }

    private:
      Napi::Promise::Deferred promise;
      T executeResult;
      T (*execute)(Napi::Env);
     Napi::Value (*complete)(Napi::Env, const T&);
  };
}
#endif
#pragma once