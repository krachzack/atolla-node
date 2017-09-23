#include "source.h"

#include <cstring>
#include <cstdlib>

using namespace v8;
using namespace atolla;

Persistent<Function> Source::constructor;

Source::Source(const AtollaSourceSpec* spec) {
  atollaSource = atolla_source_make(spec);
}

Source::~Source() {
  atolla_source_free(atollaSource);
}

void Source::Init(Handle<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Source"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "state", State);
  NODE_SET_PROTOTYPE_METHOD(tpl, "putReadyCount", PutReadyCount);
  NODE_SET_PROTOTYPE_METHOD(tpl, "putReadyTimeout", PutReadyTimeout);
  NODE_SET_PROTOTYPE_METHOD(tpl, "put", Put);
  NODE_SET_PROTOTYPE_METHOD(tpl, "errorMsg", ErrorMsg);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "Source"),
               tpl->GetFunction());
}

void Source::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.IsConstructCall()) {
    // Invoked as constructor: `new Source(...)`

    AtollaSourceSpec spec;
    if(ParseSpecFromArgs(args, spec)) {
      Source* obj = new Source(&spec);
      obj->Wrap(args.This());
      args.GetReturnValue().Set(args.This());

      free((void*) spec.sink_hostname); // free the strdup string
      spec.sink_hostname = NULL;
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

bool Source::ParseSpecFromArgs(const FunctionCallbackInfo<Value>& args, AtollaSourceSpec& parsed) {
  Isolate* isolate = args.GetIsolate();
  
  if(args.Length() < 1) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "No source spec argument given")));
      return false;
  }

  if(!args[0]->IsObject()) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "Source spec must be an object")));
      return false;
  }

  Local<Context> context = isolate->GetCurrentContext();
  Local<Object> spec = args[0]->ToObject(context).ToLocalChecked();
  
  Local<Value> hostnameVal = spec->Get(context, String::NewFromUtf8(isolate, "hostname")).ToLocalChecked();
  if(!hostnameVal->IsString()) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "hostname property must have a value of type String")));
      return false;
  }

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

  Local<Value> frameDurationVal = spec->Get(context, String::NewFromUtf8(isolate, "frameDurationMs")).ToLocalChecked();
  if(!frameDurationVal->IsNumber()) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "frameDurationMs property must have a value of type Number")));
      return false;
  } else if(((unsigned int) frameDurationVal->NumberValue()) < 5 || ((unsigned int) frameDurationVal->NumberValue()) > 255) {
      isolate->ThrowException(
          Exception::TypeError(
              String::NewFromUtf8(isolate, "frameDurationMs property must be in range 5..255")));
      return false;
  }

  MaybeLocal<Value> maxBufferedFramesMaybeVal = spec->Get(context, String::NewFromUtf8(isolate, "maxBufferedFrames"));
  int maxBufferedFrames;
  if(maxBufferedFramesMaybeVal.IsEmpty()) {
      // Let implementation pick default value
      maxBufferedFrames = 0;
  } else {
      Local<Value> maxBufferedFramesVal = maxBufferedFramesMaybeVal.ToLocalChecked();

      if(maxBufferedFramesVal->IsUndefined() || maxBufferedFramesVal->IsNull()) {
          // Let implementation pick default value
          maxBufferedFrames = 0;
      } else if(!maxBufferedFramesVal->IsNumber()) {
          isolate->ThrowException(
              Exception::TypeError(
                  String::NewFromUtf8(isolate, "maxBufferedFrames property must have a value of type Number")));
          return false;
      } else {
          maxBufferedFrames = (int) maxBufferedFramesVal->NumberValue();
          if(maxBufferedFrames < 0 || maxBufferedFrames > 255)
          {
              isolate->ThrowException(
                  Exception::TypeError(
                      String::NewFromUtf8(isolate, "maxBufferedFrames property must be in range 0..255")));
              return false;
          }
      }
  }

  MaybeLocal<Value> retryTimeoutMsMaybeVal = spec->Get(context, String::NewFromUtf8(isolate, "retryTimeout"));
  int retryTimeout;
  if(retryTimeoutMsMaybeVal.IsEmpty()) {
      // Let implementation pick default value
      retryTimeout = 0;
  } else {
      Local<Value> retryTimeoutMsVal = retryTimeoutMsMaybeVal.ToLocalChecked();

      if(retryTimeoutMsVal->IsUndefined() || retryTimeoutMsVal->IsNull()) {
          // Let implementation pick default value
          retryTimeout = 0;
      } else if(!retryTimeoutMsVal->IsNumber()) {
          isolate->ThrowException(
              Exception::TypeError(
                  String::NewFromUtf8(isolate, "retryTimeout property must have a value of type Number")));
          return false;
      } else {
          retryTimeout = (int) retryTimeoutMsVal->NumberValue();
          if(retryTimeout < 0 || retryTimeout > 10000)
          {
              isolate->ThrowException(
                  Exception::TypeError(
                      String::NewFromUtf8(isolate, "retryTimeout property must be in range 0..10000")));
              return false;
          }
      }
  }

  MaybeLocal<Value> disconnectTimeoutMaybeVal = spec->Get(context, String::NewFromUtf8(isolate, "disconnectTimeout"));
  int disconnectTimeout;
  if(disconnectTimeoutMaybeVal.IsEmpty()) {
      // Let implementation pick default value
      disconnectTimeout = 0;
  } else {
      Local<Value> disconnectTimeoutVal = disconnectTimeoutMaybeVal.ToLocalChecked();

      if(disconnectTimeoutVal->IsUndefined() || disconnectTimeoutVal->IsNull()) {
          // Let implementation pick default value
          disconnectTimeout = 0;
      } else if(!disconnectTimeoutVal->IsNumber()) {
          isolate->ThrowException(
              Exception::TypeError(
                  String::NewFromUtf8(isolate, "disconnectTimeout property must have a value of type Number")));
          return false;
      } else {
          disconnectTimeout = (int) disconnectTimeoutVal->NumberValue();
          if(disconnectTimeout < 0 || disconnectTimeout > 100000)
          {
              isolate->ThrowException(
                  Exception::TypeError(
                      String::NewFromUtf8(isolate, "disconnectTimeout property must be in range 0..100000")));
              return false;
          }
      }
  }

  parsed.sink_hostname = strdup(*String::Utf8Value(hostnameVal->ToString()));
  parsed.sink_port = (int) portVal->NumberValue();
  parsed.frame_duration_ms = (int) frameDurationVal->NumberValue();
  parsed.max_buffered_frames = maxBufferedFrames;
  parsed.retry_timeout_ms = retryTimeout;
  parsed.disconnect_timeout_ms = disconnectTimeout;
  parsed.async_make = true;

  return true;
}

void Source::State(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Source* obj = ObjectWrap::Unwrap<Source>(args.Holder());

  AtollaSourceState state = atolla_source_state(obj->atollaSource);
  const char* stateStr = NULL;
  switch(state) {
    case ATOLLA_SOURCE_STATE_OPEN:
        stateStr = "ATOLLA_SOURCE_STATE_OPEN";
        break;

    case ATOLLA_SOURCE_STATE_WAITING:
        stateStr = "ATOLLA_SOURCE_STATE_WAITING";
        break;

    case ATOLLA_SOURCE_STATE_ERROR:
        stateStr = "ATOLLA_SOURCE_STATE_ERROR";
        break;
  }

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, stateStr));
}

void Source::ErrorMsg(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
  
    Source* obj = ObjectWrap::Unwrap<Source>(args.Holder());
  
    const char* msg = atolla_source_error_msg(obj->atollaSource);
  
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, msg));
}

void Source::PutReadyCount(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
  
    Source* obj = ObjectWrap::Unwrap<Source>(args.Holder());
  
    double count = atolla_source_put_ready_count(obj->atollaSource);
  
    args.GetReturnValue().Set(Number::New(isolate, count));
}

void Source::PutReadyTimeout(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
  
    Source* obj = ObjectWrap::Unwrap<Source>(args.Holder());
  
    double timeout = atolla_source_put_ready_timeout(obj->atollaSource);
  
    args.GetReturnValue().Set(Number::New(isolate, timeout));
}

void Source::Put(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
  
    Source* obj = ObjectWrap::Unwrap<Source>(args.Holder());

    if(args.Length() < 1) {
        isolate->ThrowException(
            Exception::TypeError(
                String::NewFromUtf8(isolate, "No frame argument given")));
        return;
    }

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
    if (ui8_length > 0)
      assert(ui8_data != nullptr);

    bool ok = atolla_source_put(obj->atollaSource, ui8_data, ui8_length);
  
    args.GetReturnValue().Set(Boolean::New(isolate, ok));
}
