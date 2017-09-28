#include "sink.h"

#include <cstring>
#include <cstdlib>

using namespace v8;
using namespace atolla;

Persistent<Function> Sink::constructor;

Sink::Sink(const AtollaSinkSpec* spec) {
  frameLen = spec->lights_count * 3;
  atollaSink = atolla_sink_make(spec);
}

Sink::~Sink() {
  atolla_sink_free(atollaSink);
}

void Sink::Init(Handle<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Sink"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "state", State);
  NODE_SET_PROTOTYPE_METHOD(tpl, "errorMsg", ErrorMsg);
  NODE_SET_PROTOTYPE_METHOD(tpl, "get", Get);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "Sink"),
               tpl->GetFunction());
}

void Sink::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.IsConstructCall()) {
    // Invoked as constructor: `new Sink(...)`

    AtollaSinkSpec spec;
    if(ParseSpecFromArgs(args, spec)) {
      Sink* obj = new Sink(&spec);
      obj->Wrap(args.This());
      args.GetReturnValue().Set(args.This());
    } else {
        return; // Already thrown exception, just exit
    }
  } else {
    isolate->ThrowException(
        Exception::TypeError(
            String::NewFromUtf8(isolate, "Sink constructor was called without new operator")));

    return;
  }
}

bool Sink::ParseSpecFromArgs(const FunctionCallbackInfo<Value>& args, AtollaSinkSpec& parsed) {
  Isolate* isolate = args.GetIsolate();

  if(args.Length() < 1) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "No sink spec argument given")));
      return false;
  }

  if(!args[0]->IsObject()) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "Sink spec must be an object")));
      return false;
  }

  Local<Context> context = isolate->GetCurrentContext();
  Local<Object> spec = args[0]->ToObject(context).ToLocalChecked();

  Local<Value> portVal = spec->Get(context, String::NewFromUtf8(isolate, "port")).ToLocalChecked();
  if(!portVal->IsNumber()) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "port property must have a value of type Number")));
      return false;
  } else if(((unsigned int) portVal->NumberValue()) < 1024 || ((unsigned int) portVal->NumberValue()) > 65535) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "port property must be in range 1024..65535")));
      return false;
  }

  Local<Value> lightsCountVal = spec->Get(context, String::NewFromUtf8(isolate, "lightsCount")).ToLocalChecked();
  if(!lightsCountVal->IsNumber()) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "lightsCount property must have a value of type Number")));
      return false;
  } else if(((unsigned int) lightsCountVal->NumberValue()) < 1 || ((unsigned int) lightsCountVal->NumberValue()) > 300) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "lightsCount property must be in range 1..300")));
      return false;
  }

  parsed.port = (int) portVal->NumberValue();
  parsed.lights_count = (int) lightsCountVal->NumberValue();

  return true;
}

void Sink::State(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Sink* obj = ObjectWrap::Unwrap<Sink>(args.Holder());

  AtollaSinkState state = atolla_sink_state(obj->atollaSink);
  const char* stateStr = NULL;
  switch(state) {
    case ATOLLA_SINK_STATE_LENT:
        stateStr = "ATOLLA_SINK_STATE_LENT";
        break;

    case ATOLLA_SINK_STATE_OPEN:
        stateStr = "ATOLLA_SINK_STATE_OPEN";
        break;

    case ATOLLA_SINK_STATE_ERROR:
        stateStr = "ATOLLA_SINK_STATE_ERROR";
        break;
  }

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, stateStr));
}

void Sink::ErrorMsg(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    Sink* obj = ObjectWrap::Unwrap<Sink>(args.Holder());

    const char* msg = atolla_sink_error_msg(obj->atollaSink);

    args.GetReturnValue().Set(String::NewFromUtf8(isolate, msg));
}

void Sink::Get(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    Sink* obj = ObjectWrap::Unwrap<Sink>(args.Holder());

    if(!args[0]->IsUint8Array()) {
        isolate->ThrowException(
            Exception::TypeError(
                String::NewFromUtf8(isolate, "Frame argument is not a Uint8Array")));
        return;
    }

    Local<Uint8Array> ui8 = args[0].As<Uint8Array>();
    v8::ArrayBuffer::Contents ui8_c = ui8->Buffer()->GetContents();
    const size_t ui8_offset = ui8->ByteOffset();
    const size_t ui8_length = ui8->ByteLength();
    char* const ui8_data = static_cast<char*>(ui8_c.Data()) + ui8_offset;

    bool ok = atolla_sink_get(obj->atollaSink, ui8_data, ui8_length);

    args.GetReturnValue().Set(Boolean::New(isolate, ok));
}
