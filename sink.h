#ifndef SINK_H
#define SINK_H

#include <node.h>
#include <node_object_wrap.h>

#include "lib/atolla/atolla/sink.h"

namespace atolla {
  class Sink : public node::ObjectWrap {
  public:
    static void Init(v8::Handle<v8::Object> exports);

  private:
    explicit Sink(const AtollaSinkSpec* spec);
    ~Sink();

    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void State(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ErrorMsg(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Get(const v8::FunctionCallbackInfo<v8::Value>& args);
    static v8::Persistent<v8::Function> constructor;

    static bool ParseSpecFromArgs(const v8::FunctionCallbackInfo<v8::Value>& args, AtollaSinkSpec& spec);

    AtollaSink atollaSink;
    size_t frameLen;
  };
}

#endif // SINK_H