#include <node.h>
#include "lib/atolla/atolla/version.h"

#include "source.h"
#include "sink.h"

namespace atolla {

    using v8::Local;
    using v8::String;
    using v8::Number;
    using v8::Object;
    using v8::Handle;
    using v8::Isolate;

    void SetVersion(Handle<Object> exports);

    void init(Local<Object> exports) {
        SetVersion(exports);
        Sink::Init(exports);
        Source::Init(exports);
    }

    void SetVersion(Handle<Object> exports) {
        Isolate* isolate = Isolate::GetCurrent();

        // Set library version according to header
        exports->Set(
            String::NewFromUtf8(isolate, "versionLibraryMajor"),
            Number::New(isolate, ATOLLA_VERSION_LIBRARY_MAJOR)
        );
        exports->Set(
            String::NewFromUtf8(isolate, "versionLibraryMinor"),
            Number::New(isolate, ATOLLA_VERSION_LIBRARY_MINOR)
        );
        exports->Set(
            String::NewFromUtf8(isolate, "versionLibraryPatch"),
            Number::New(isolate, ATOLLA_VERSION_LIBRARY_PATCH)
        );

        // Set protocol version implemented by the library according to header
        exports->Set(
            String::NewFromUtf8(isolate, "versionProtocolMajor"),
            Number::New(isolate, ATOLLA_VERSION_PROTOCOL_MAJOR)
        );
        exports->Set(
            String::NewFromUtf8(isolate, "versionProtocolMinor"),
            Number::New(isolate, ATOLLA_VERSION_PROTOCOL_MINOR)
        );
    }

    NODE_MODULE(atolla, init)

}  // namespace atolla
