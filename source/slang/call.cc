#include "call.h"
#include "types/basic_types.h"

namespace xynq {
namespace slang {

// Field type.
TypeSchema k_slang_field_type = {
    "Field",
    alignof(Field),
    sizeof(Field)
};

TypeSchemaPtr k_slang_field_type_ptr = &k_slang_field_type;
////////////////////////////////////////////////////////////


} // slang
} // xynq
