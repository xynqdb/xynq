#include "file_stream.h"

using namespace xynq;

bool InFileStream::Open(CStrSpan file_path) {
    name_ = file_path;
    file_.open(file_path.CStr());
    file_.rdbuf()->pubsetbuf(0, 0); // we're doing our own buffering.
    return file_.is_open();
}

bool InFileStream::IsValid() const {
    return file_.is_open();
}

Either<StreamError, size_t> InFileStream::DoRead(MutDataSpan read_buf) {
    file_.read((char *)read_buf.Data(), (std::streamsize)read_buf.Size());
    if (file_.gcount() > 0) {
        return (size_t)file_.gcount();
    }

    return file_.eof() ? StreamError::Closed : StreamError::IOError;
}
