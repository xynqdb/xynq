#pragma once

#include "stream.h"

#include <fstream>

namespace xynq {

// Input file stream.
class InFileStream final : public InStream {
public:
    // Opens file for read.
    // File will be closed on destruction.
    // true if opened successfully, false otherwise.
    bool Open(CStrSpan file_path);

    // Is underlying file opened and ready to be read.
    bool IsValid() const;
private:
    std::ifstream file_;

    Either<StreamError, size_t> DoRead(MutDataSpan read_buf) final;
};

} // xynq