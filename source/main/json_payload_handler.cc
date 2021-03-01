#include "json_payload_handler.h"

#include "json/json_deserializer.h"

using namespace xynq;

JsonPayloadHandler::JsonPayloadHandler(Dep<Storage> storage)
    : storage_(storage)
{}

slang::PayloadResult JsonPayloadHandler::ProcessPayload(StreamReader &reader, ScratchAllocator &allocator) {
    // auto get_object_writer = [&](StrSpan type_name) -> Either<StrSpan, StreamWriter> {
    //     return storage_->CreateTempObject(type_name)
    //         .MapRight([](Object::Handle obj) {
    //             return obj->Writer();
    //         });
    // };

    // JsonDeserializer deser(allocator, get_object_writer);
    // return deser.Deserialize(reader).MapRight([](JsonDeserializerSuccess) {
    //     return slang::PayloadSuccess{};
    // });
    return StrSpan{"Not supported"};
}
