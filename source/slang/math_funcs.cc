#include "math_funcs.h"
#include "call.h"

using namespace xynq;
using namespace xynq::slang;

namespace {

// Summation.
template<class T>
T sum(CallArgs &args) {
    T result = 0;
    auto it = args.Begin();
    while (!it.IsEnd()) {
        result += it.Get<T>().Value();
        ++it;
    }
    return result;
}

// Substraction
template<class T>
T sub(CallArgs &args) {
    T res = 0;
    auto it = args.Begin();
    if (!it.IsEnd()) {
        res = it.Get<T>().Value();
        ++it;

        while (!it.IsEnd()) {
            res -= it.Get<T>().Value();
            ++it;
        }
    }

    return res;
}

// Multiplication.
template<class T>
T mul(CallArgs &args) {
    T res = 1;
    auto it = args.Begin();
    while (!it.IsEnd()) {
        res *= it.Get<T>().Value();
        ++it;
    }

    return res;
}

// Division.
double div(CallArgs &args) {
    auto it = args.Begin();
    if (it.IsEnd()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double d0 = it.Get<double>().Value();
    ++it;

    if (it.IsEnd()) {
        return 1.0 / d0;
    }

    double d1 = 1.0;
    while (!it.IsEnd()) {
        d1 *= it.Get<double>().Value();
        ++it;
    }

    return d0 / d1;
}
////////////////////////////////////////////////////////////

enum MathOpType {
    Invalid,
    SignedInt,
    Double
};

MathOpType CheckOperationType(CallArgs &args) {
    bool is_float = false;
    auto it = args.Begin();
    while (!it.IsEnd() && it.Type()->IsNumeric()) {
        is_float = is_float || it.Type()->IsFloatingPoint();
        ++it;
    }

    if (!it.IsEnd()) {
        return MathOpType::Invalid;
    }

    return is_float ? MathOpType::Double : MathOpType::SignedInt;
}

const char *k_invalid_type_error = "Operation expects numeric type";

} // anon


void xynq::slang::RegisterMathFunctions(FuncTable &func_table) {
    func_table["+"] = [](slang::CallContext &call_context) {
        switch (CheckOperationType(*call_context.args)) {
            case MathOpType::Invalid:
                call_context.error_text.Append(k_invalid_type_error);
                return false;
            case MathOpType::Double:
                call_context.output->Add(sum<double>(*call_context.args));
                return true;
            case MathOpType::SignedInt:
                call_context.output->Add(sum<int64_t>(*call_context.args));
                return true;
        }
    };

    func_table["-"] = [](slang::CallContext &call_context) {
        switch (CheckOperationType(*call_context.args)) {
            case MathOpType::Invalid:
                call_context.error_text.Append(k_invalid_type_error);
                return false;
            case MathOpType::Double:
                call_context.output->Add(sub<double>(*call_context.args));
                return true;
            case MathOpType::SignedInt:
                call_context.output->Add(sub<int16_t>(*call_context.args));
                return true;
        }
    };

    func_table["*"] = [](slang::CallContext &call_context) {
        switch (CheckOperationType(*call_context.args)) {
            case MathOpType::Invalid:
                call_context.error_text.Append(k_invalid_type_error);
                return false;
            case MathOpType::Double:
                call_context.output->Add(mul<double>(*call_context.args));
                return true;
            case MathOpType::SignedInt:
                call_context.output->Add(mul<int16_t>(*call_context.args));
                return true;
        }
    };

    func_table["/"] = [](slang::CallContext &call_context) {
        switch (CheckOperationType(*call_context.args)) {
            case MathOpType::Invalid:
                call_context.error_text.Append(k_invalid_type_error);
                return false;
            case MathOpType::Double:
            case MathOpType::SignedInt:
                call_context.output->Add(div(*call_context.args));
                return true;
        }
    };
}