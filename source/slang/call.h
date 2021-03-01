#pragma once

#include "base/maybe.h"
#include "base/span.h"
#include "base/str_builder.h"
#include "containers/vec.h"
#include "types/basic_types.h"
#include "types/schema.h"
#include "types/value_types.h"

#include <stdio.h>

namespace xynq {

class StreamWriter;

namespace slang {

class CallArgs;
class CallOutput;

struct Field : public StrSpan {};
extern TypeSchemaPtr k_slang_field_type_ptr;

enum class OpCode : uint8_t {
    Invalid = 0,
    Push,           // Push data on stack
    Call,           // Call a function (data.ptr is a function pointer)
};

struct CallContext {
    CallArgs *args = nullptr;
    CallOutput *output = nullptr;
    StrBuilder<128> error_text;

    void *user_data = nullptr;

    template<class T>
    inline T &UserData() {
        T *ptr = reinterpret_cast<T*>(user_data);
        XYAssert(ptr != nullptr);
        return *ptr;
    }
};

using Call = bool(*)(CallContext &);
using StackType = ScratchVec<TypedValue>;

// Writer of a function result.
// Output should only be written after all arguments are processed or not needed cause
// it will purge function arguments in the stack.
class CallOutput {
    friend class Program;

public:
    template<class T>
    void Add(T value);

    // Add schemed type.
    template<class T>
    void AddTyped(TypeSchemaPtr type, T value);

private:
    StackType &stack_;

    CallOutput(ScratchVec<TypedValue> &stack)
        : stack_(stack)
    {}
};

// List of arguments for a function call.
class CallArgs {
    friend class Program;
public:

    // Iterator over arguments.
    struct Iterator {
        friend class CallArgs;

        // True if this iterator is beyond arguments list.
        // the same as == end() with stl.
        bool IsEnd() const {
            XYAssert(arg_ != nullptr);
            return arg_->type == XYBasicType(FrameBarrier);
        }

        // Move to next argument.
        Iterator& operator++() {
            --arg_;
            return *this;
        }

        // Get argument with certain type.
        // If there's type mismatch - maybe will be empty.
        template<class T>
        Maybe<T> Get() const;

        // Same as Get() but not checking types.
        template<class T>
        constexpr T GetUnsafe() const;

        // Returns argument type.
        TypeSchemaPtr Type() const { return arg_->type; }

        // Returns argument value.
        Value Value() const { return arg_->value; }
    private:
        Iterator(const TypedValue *arg)
            : arg_(arg)
        {}

        const TypedValue *arg_;
    };

    // Returns iterator to the first argument of a function.
    Iterator Begin() { return args_.Data() + args_.Size() - 1; }
private:
    Span<TypedValue> args_;

    CallArgs(Span<TypedValue> args)
        : args_(args)
    {}
};

struct Instruction {
    OpCode code;
    TypedValue data;

    Instruction(OpCode code_, TypedValue data_)
        : code(code_)
        , data(data_)
    {}
};
////////////////////////////////////////////////////////////


// Implementation.
template<class T>
void CallOutput::Add(T value) {
    AddTyped(GetBasicType<T>(), value);
}

template<class T>
void CallOutput::AddTyped(TypeSchemaPtr type, T value) {
    stack_.emplace_back(type, value);
}

template<class T>
constexpr T CallArgs::Iterator::GetUnsafe() const {
    if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(arg_->value.dbl);
    } else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        return static_cast<T>(arg_->value.i64);
    } else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
        return static_cast<T>(arg_->value.u64);
    } else if constexpr (std::is_same_v<T, StrSpan>) {
        return arg_->value.str;
    } else if constexpr (std::is_same_v<T, Field>) {
        return Field{arg_->value.str};
    }

    XYAssert(false);
    return {};
}
template<class T>
Maybe<T> CallArgs::Iterator::Get() const {
    if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
        if (arg_->type->IsUnsignedInt()) {
            return static_cast<T>(arg_->value.u64);
        } if (arg_->type->IsSignedInt()) {
            return static_cast<T>(arg_->value.i64);
        } else if (arg_->type->IsFloatingPoint()) {
            return static_cast<T>(arg_->value.dbl);
        }
    } else if constexpr (std::is_same_v<T, StrSpan>) {
        if (arg_->type == XYBasicType(StrSpan)) {
            return arg_->value.str;
        }
    } else if constexpr (std::is_same_v<T, Field>) {
        if (arg_->type == k_slang_field_type_ptr) {
            return Field{arg_->value.str};
        }
    } else {
        XYAssert(false);
    }

    return {};
}

} // slang
} // xynq