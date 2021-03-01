#pragma once

#include "assert.h"
#include "either.h"
#include "span.h"

namespace xynq {

// Possible read/write errors.
enum class StreamError {
    // No error.
    None,

    // Stream was closed. (ie. underlying network connection disconnected)
    Closed,

    // Error at the IO/Network layer.
    IOError
};

enum class StreamWriteSuccess {};
using StreamWriteResult = Either<StreamError, StreamWriteSuccess>;

// Input stream.
class InStream {
public:
    CStrSpan Name() const { return name_; }
    StreamError LastError() const { return read_error_; }
    Either<StreamError, size_t> Read(MutDataSpan read_buf) {
        return DoRead(read_buf).MapLeft([this](StreamError error) {
            read_error_ = error;
            return error;
        });
    }

protected:
    CStrSpan name_ = "n/a"; // Name for debugging/logging.
    StreamError read_error_ = StreamError::None;

    virtual Either<StreamError, size_t> DoRead(MutDataSpan read_buf) = 0;

    // Only allow destroying from the parent class.
    // So we don't need a virtual d-tor here.
    ~InStream() = default;
};

// Output stream.
class OutStream {
public:
    CStrSpan Name() const { return name_; }
    StreamError LastError() const { return write_error_; }
    StreamWriteResult Write(DataSpan write_buf) {
        return DoWrite(write_buf).MapLeft([this](StreamError error) {
            write_error_ = error;
            return error;
        });
    }

protected:
    CStrSpan name_ = "n/a"; // Name for debugging/logging.
    StreamError write_error_ = StreamError::None;

    virtual StreamWriteResult DoWrite(DataSpan write_buf) = 0;

    // Only allow destroying from the parent class.
    // So we don't need a virtual d-tor here.
    ~OutStream() = default;
};

// Full-duplex stream.
class InOutStream : public InStream, public OutStream {};
////////////////////////////////////////////////////////////


// RAII style stream reader with pre-buffering.
class StreamReader {
public:
    // RAII-style constructor.
    // buffer and stream are expected to stay alive for as long as this reader is used.
    StreamReader(MutDataSpan buffer, InStream &stream);

    // Constructor that allows setting some bytes that already preloaded.
    StreamReader(MutDataSpan buffer, InStream &stream, size_t available_bytes);

    // Underlying stream.
    InStream &Stream();

    // Returns true if the udnerlying stream is readable,
    // Otherwise false. That means the stream is either closed or some IO error happened.
    bool IsGood() const;

    // Returns currently available buffered data.
    MutDataSpan Available();
    DataSpan Available() const;

    // Returns currently available buffered data or if nothing buffered -
    // tries to read new chunk of data from the stream.
    Either<StreamError, MutDataSpan> AvailableOrRead();
    // Same as AvailableOrRead but will also advance internal buffer after returning
    // available.
    Either<StreamError, MutDataSpan> DrainOrRead();


    // Increments current read offset by add_offset.
    void Advance(size_t add_offset);

    // Enforces prebuffering, resets all data that currently is prebuffered.
    Either<StreamError, MutDataSpan> RefillAvailable();

    // Reads one charactar from the buffer without checking for buffer bounds.
    inline char ReadAvailableCharUnsafe();

    // Reads typed value. Stream buffer must be aligned to alignof(T).
    template<class T>
    Either<StreamError, T> ReadValue();

private:
    MutDataSpan read_buf_; // User buffer used for prebuffering data from the stream.
    InStream &stream_;
    uint8_t *available_begin_ = nullptr;
    uint8_t *available_end_ = nullptr;

    MutDataSpan NormalizeAvailable();
};

// RAII style stream writer with pre-buffering.
class StreamWriter {
public:
    // Stream is expected to stay alive for as long as this reader is used.
    StreamWriter(MutDataSpan buffer, OutStream &stream);
    // Allows setting buffer that already has some data in it.
    // After initialization buffer is offseted by written_size.
    StreamWriter(MutDataSpan buffer, OutStream &stream, size_t written_size);
    // Will flush currently prebuffered data into the stream.
    ~StreamWriter();

    // Underlying stream.
    OutStream &Stream();

    // Returns true if the udnerlying stream is writeable,
    // Otherwise false. That means the stream is either closed or some IO error happened.
    bool IsGood() const;

    // Writes buffer of data into the stream.
    StreamWriteResult WriteData(DataSpan buf);

    // Writers string into the stream.
    StreamWriteResult Write(StrSpan str);

    template<class T>
    StreamWriteResult Write(const T &value);

    // Flushes data into the stream.
    // ie. if underlying stream is network i/o -> will write or schedule writing to the wire.
    StreamWriteResult Flush();
private:
    MutDataSpan write_buf_; // User buffer used for prebuffering data from the stream.
    OutStream &stream_;
    size_t written_size_; // Currently written to the buffer.
};
////////////////////////////////////////////////////////////


// Stream with no real input.
// Will return StreamError::Closed on attempt to read out of buffer bounds.
class DummyInStream final : public InStream {
public:
    Either<StreamError, size_t> DoRead(MutDataSpan read_buf) final;
};

// Will return StreamError::Closed on attempt to write.
class DummyOutStream final : public OutStream {
public:
    StreamWriteResult DoWrite(DataSpan write_buf) final;
};
////////////////////////////////////////////////////////////



// StreamReader.
template<class T>
Either<StreamError, T> StreamReader::ReadValue() {
    XYAssert(sizeof(T) <= read_buf_.Size()); // if T is smaller than buffer size -> we would never be able to read it.

    while (available_begin_ + sizeof(T) > available_end_) {
        MutDataSpan buf = NormalizeAvailable();
        auto res = stream_.Read(buf);
        if (res.IsLeft()) {
            return res.Left();
        }

        available_begin_ = (uint8_t *)read_buf_.Data();
        available_end_ = available_end_ + res.Right();
    }

    XYAssert(((uintptr_t)available_begin_ % alignof(T)) == 0);
    T *value = reinterpret_cast<T *>(available_begin_);
    available_begin_ += sizeof(T);
    return *value;
}

char StreamReader::ReadAvailableCharUnsafe() {
    XYAssert(available_begin_ != available_end_);
    return *available_begin_++;
}


// StreamWriter.
template<class T>
StreamWriteResult StreamWriter::Write(const T &value) {
    return WriteData(DataSpan{&value, sizeof(value)});
}

} // xynq