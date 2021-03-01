#include "stream.h"

#include <algorithm>

using namespace xynq;

// StreamReader.
StreamReader::StreamReader(MutDataSpan buffer, InStream &stream)
    : read_buf_(buffer)
    , stream_(stream)
{}

StreamReader::StreamReader(MutDataSpan buffer, InStream &stream, size_t available_bytes)
    : StreamReader(buffer, stream) {
    XYAssert(available_bytes <= buffer.Size());
    available_begin_ = (uint8_t *)buffer.Data();
    available_end_ = (uint8_t *)buffer.Data() + available_bytes;
}

InStream &StreamReader::Stream() {
    return stream_;
}

bool StreamReader::IsGood() const {
    return stream_.LastError() == StreamError::None;
}

MutDataSpan StreamReader::Available() {
    return MutDataSpan{available_begin_, available_end_};
}

DataSpan StreamReader::Available() const {
    return DataSpan{available_begin_, available_end_};
}

Either<StreamError, MutDataSpan> StreamReader::AvailableOrRead() {
    // Have some data prebuffered.
    if (available_begin_ != available_end_) {
        return Available();
    }

    // Nothing prebuffered -> try read from stream.
    return stream_.Read(read_buf_).MapRight([&](size_t read_size) {
        available_begin_ = (uint8_t *)read_buf_.Data();
        available_end_ = (uint8_t *)read_buf_.Data() + read_size;
        return MutDataSpan{available_begin_, available_end_};
    });
}

Either<StreamError, MutDataSpan> StreamReader::DrainOrRead() {
    // Have some data prebuffered.
    if (available_begin_ != available_end_) {
        auto result = MutDataSpan{available_begin_, available_end_};
        available_begin_ = available_end_;
        return result;
    }

    // Nothing prebuffered -> try read from stream.
    return stream_.Read(read_buf_).MapRight([&](size_t read_size) {
        return MutDataSpan{read_buf_.Data(), read_size};
    });
}

void StreamReader::Advance(size_t add_offset) {
    XYAssert(add_offset <= Available().Size());
    available_begin_ += add_offset;
}

MutDataSpan StreamReader::NormalizeAvailable() {
    if (available_begin_ != read_buf_.Data()) {
        size_t sz = available_end_ - available_begin_;
        memmove(read_buf_.Data(), available_begin_, sz);
        available_begin_ = (uint8_t *)read_buf_.Data();
        available_end_ = available_begin_ + sz;
    }

    return MutDataSpan{available_begin_, available_end_};
}

Either<StreamError, MutDataSpan> StreamReader::RefillAvailable() {
    return stream_.Read(read_buf_).Fold([&](StreamError error) -> Either<StreamError, MutDataSpan> {
        available_begin_ = available_end_;
        return error;
    }, [&](size_t num_read) -> Either<StreamError, MutDataSpan> {
        available_begin_ = (uint8_t *)read_buf_.Data();
        available_end_ = (uint8_t *)read_buf_.Data() + num_read;
        return MutDataSpan{read_buf_.Data(), num_read};
    });
}


// Stream writer.
StreamWriter::StreamWriter(MutDataSpan buffer, OutStream &stream)
    : write_buf_(buffer)
    , stream_(stream)
    , written_size_(0)
{}

StreamWriter::StreamWriter(MutDataSpan buffer, OutStream &stream, size_t written_size)
    : StreamWriter(buffer, stream) {
    written_size_ = written_size;
}

StreamWriter::~StreamWriter() {
    Flush();
}

OutStream &StreamWriter::Stream() {
    return stream_;
}

bool StreamWriter::IsGood() const {
    return stream_.LastError() == StreamError::None;
}

StreamWriteResult StreamWriter::WriteData(DataSpan buf) {
    const char *ptr = (const char *)buf.Data();
    const char *ptr_end = (const char *)buf.Data() + buf.Size();

    while (ptr != ptr_end) {
        size_t sz = std::min((size_t)(ptr_end - ptr), write_buf_.Size() - written_size_);
        memcpy((char *)write_buf_.Data() + written_size_, ptr, sz);

        ptr += sz;
        written_size_ += sz;

        if (ptr != ptr_end) {
            auto result = Flush();
            if (result.IsLeft()) {
                return result;
            }
        }
    }

    return StreamWriteSuccess{};
}

StreamWriteResult StreamWriter::Write(StrSpan str) {
    return WriteData(DataSpan{str.Data(), str.Size()});
}

Either<StreamError, StreamWriteSuccess> StreamWriter::Flush() {
    size_t sz = written_size_;
    written_size_ = 0;
    return stream_.Write({write_buf_.Data(), sz});
}

// DummyInStream.
Either<StreamError, size_t> DummyInStream::DoRead(MutDataSpan /*read_buf*/) {
    return StreamError::Closed;
}

// DummyOutStream.
StreamWriteResult DummyOutStream::DoWrite(DataSpan /*write_buf*/) {
    return StreamError::Closed;
}
