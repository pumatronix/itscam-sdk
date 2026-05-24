/*
 *  itscam_multipart.h
 *
 *  ITSCAM Client SDK - Multipart parser (internal)
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Lightweight RFC-2046 multipart parser tailored for the two server-side
 *  shapes used by the daemon CGIs:
 *
 *    - "multipart/related; boundary=snapshot"
 *           used by snapshot.cgi when returning multi-exposure groups
 *
 *    - "multipart/x-mixed-replace; boundary=MjpegBoundary"
 *           used by mjpegvideo.cgi for streaming JPEG frames
 *
 *  The implementation is buffer-based (parseAll) for snapshot.cgi and
 *  streaming (StreamingMultipartParser) for mjpegvideo.cgi.
 */
#pragma once

#include "../itscam_types.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace itscam {
namespace detail {

//=========================================================================
// Boundary extraction
//=========================================================================

/// Pull the `boundary=...` parameter out of a Content-Type header.  Returns
/// the empty string when no boundary is present.  Both quoted and unquoted
/// forms are accepted.
std::string extractMultipartBoundary(const std::string& contentType);

//=========================================================================
// Buffered parse  (snapshot.cgi)
//=========================================================================

/// Parse @p body as a multipart payload using @p boundary.  Each part is
/// converted into a CgiImage with its `headers` map filled and `mimeType`
/// taken from the part's Content-Type.  Returns an empty vector if no
/// parts can be extracted.
std::vector<CgiImage> parseMultipart(const std::vector<uint8_t>& body,
                                     const std::string& boundary);

//=========================================================================
// Streaming parse  (mjpegvideo.cgi, snapshot multipart over the wire)
//=========================================================================

/// Incremental multipart parser.  Push bytes via push() as they arrive and
/// each completed part is delivered through the callback supplied at
/// construction.  Stateful and not thread-safe; create one per stream.
class StreamingMultipartParser {
public:
    using PartCallback = std::function<void(const CgiImage&)>;

    explicit StreamingMultipartParser(const std::string& boundary,
                                      PartCallback cb);

    /// Feed @p length bytes from @p data into the parser.  May trigger zero
    /// or more callback invocations before returning.  Returns false if the
    /// caller should abort the stream (currently always returns true).
    bool push(const uint8_t* data, size_t length);

    /// Reset the parser state (e.g. when reconnecting).
    void reset();

private:
    std::string             mBoundary;
    std::string             mDashBoundary;   // "--" + boundary
    PartCallback            mCallback;

    enum class State {
        SeekFirstBoundary,   // skipping preamble before the first boundary
        ReadHeaders,         // accumulating part headers until CRLFCRLF
        ReadBody,            // accumulating body until next boundary
        Done                 // saw closing boundary "--<bnd>--"
    };

    State                   mState = State::SeekFirstBoundary;
    std::vector<uint8_t>    mBuffer;          // sliding window
    std::map<std::string, std::string> mHeaders;
    size_t                  mBodyExpected = 0; // Content-Length, 0 = unknown

    bool processBuffer();
    bool parseHeadersFromBuffer(size_t headersEnd);
    void emitPart(std::vector<uint8_t>&& body);
};

}  // namespace detail
}  // namespace itscam
