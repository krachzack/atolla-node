#ifndef SOURCE_H
#define SOURCE_H

#include <node.h>
#include <node_object_wrap.h>

#include "lib/atolla/atolla/source.h"

namespace atolla {
  class Source : public node::ObjectWrap {
  public:
    static void Init(v8::Handle<v8::Object> exports);

  private:
    explicit Source(const AtollaSourceSpec* spec);
    ~Source();

    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void State(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ErrorMsg(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void PutReadyCount(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void PutReadyTimeout(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Put(const v8::FunctionCallbackInfo<v8::Value>& args);
    static v8::Persistent<v8::Function> constructor;

    static bool ParseSpecFromArgs(const v8::FunctionCallbackInfo<v8::Value>& args, AtollaSourceSpec& spec);
    
    AtollaSource atollaSource;
  };
}

#endif // SOURCE_H