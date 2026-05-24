/*
 *  itscam_multipart.cpp
 *
 *  ITSCAM Client SDK - Multipart parser implementation
 *
 *  Copyright (c) 2026 Pumatronix
 */

#include "itscam_multipart.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace itscam {
namespace detail {

//=========================================================================
// Local helpers
//=========================================================================

namespace {

/// Case-insensitive substring search for an ASCII needle in @p hay.
std::string toLower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return out;
}

/// Trim ASCII whitespace from both ends.
std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

/// Locate @p needle in @p haystack starting at @p from.  Returns the
/// position of the first match, or std::string::npos.
size_t findBytes(const std::vector<uint8_t>& haystack, size_t from,
                 const std::string& needle) {
    if (needle.empty() || from > haystack.size()) return std::string::npos;
    auto it = std::search(haystack.begin() + from, haystack.end(),
                          needle.begin(), needle.end());
    return (it == haystack.end()) ? std::string::npos
                                   : static_cast<size_t>(
                                         it - haystack.begin());
}

/// Parse the headers section (between the leading "--bnd\r\n" and the
/// "\r\n\r\n" separator) into a key/value map.  Stops at the first blank
/// line or end of buffer.
std::map<std::string, std::string> parseHeaderLines(const std::string& blob) {
    std::map<std::string, std::string> out;
    size_t pos = 0;
    while (pos < blob.size()) {
        size_t eol = blob.find("\r\n", pos);
        if (eol == std::string::npos) eol = blob.size();
        std::string line = blob.substr(pos, eol - pos);
        pos = eol + 2;
        if (line.empty()) break;
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key   = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        out[key] = value;
    }
    return out;
}

/// Fetch a header value case-insensitively.
std::string headerValue(const std::map<std::string, std::string>& headers,
                        const std::string& key) {
    auto target = toLower(key);
    for (const auto& kv : headers) {
        if (toLower(kv.first) == target) return kv.second;
    }
    return {};
}

}  // namespace

//=========================================================================
// Boundary extraction
//=========================================================================

std::string extractMultipartBoundary(const std::string& contentType) {
    auto lower = toLower(contentType);
    auto pos   = lower.find("boundary=");
    if (pos == std::string::npos) return {};
    pos += 9; // strlen("boundary=")
    if (pos >= contentType.size()) return {};

    std::string value;
    if (contentType[pos] == '"') {
        ++pos;
        auto end = contentType.find('"', pos);
        value = contentType.substr(
            pos,
            (end == std::string::npos ? contentType.size() : end) - pos);
    } else {
        auto end = contentType.find_first_of("; \t", pos);
        value = contentType.substr(
            pos,
            (end == std::string::npos ? contentType.size() : end) - pos);
    }
    return trim(value);
}

//=========================================================================
// Buffered parse
//=========================================================================

std::vector<CgiImage> parseMultipart(const std::vector<uint8_t>& body,
                                     const std::string& boundary) {
    std::vector<CgiImage> out;
    if (boundary.empty() || body.empty()) return out;

    const std::string dash = "--" + boundary;

    size_t cursor = findBytes(body, 0, dash);
    if (cursor == std::string::npos) return out;

    while (cursor < body.size()) {
        // Advance past the boundary marker
        cursor += dash.size();

        // Check for closing boundary "--<bnd>--"
        if (cursor + 1 < body.size() &&
            body[cursor] == '-' && body[cursor + 1] == '-') {
            break;
        }
        // Skip optional CRLF after boundary
        if (cursor + 1 < body.size() &&
            body[cursor] == '\r' && body[cursor + 1] == '\n') {
            cursor += 2;
        }

        // Find end of headers (CRLFCRLF)
        size_t headerEnd = findBytes(body, cursor, "\r\n\r\n");
        if (headerEnd == std::string::npos) break;

        std::string headerBlob(body.begin() + cursor,
                               body.begin() + headerEnd);
        auto headers = parseHeaderLines(headerBlob + "\r\n");
        cursor = headerEnd + 4;

        // Find start of the next boundary (which delimits the body)
        size_t nextBoundary = findBytes(body, cursor, "\r\n" + dash);
        size_t bodyEnd;
        if (nextBoundary == std::string::npos) {
            // No more boundaries -- treat rest of buffer as body
            bodyEnd = body.size();
            cursor  = body.size();
        } else {
            bodyEnd = nextBoundary;
            cursor  = nextBoundary + 2; // skip the CRLF, leave at "--bnd"
        }

        CgiImage part;
        part.mimeType = headerValue(headers, "Content-Type");
        part.headers  = std::move(headers);
        part.data.assign(body.begin() + headerEnd + 4,
                         body.begin() + bodyEnd);
        out.push_back(std::move(part));
    }

    return out;
}

//=========================================================================
// StreamingMultipartParser
//=========================================================================

StreamingMultipartParser::StreamingMultipartParser(
    const std::string& boundary, PartCallback cb)
    : mBoundary(boundary)
    , mDashBoundary("--" + boundary)
    , mCallback(std::move(cb))
{}

void StreamingMultipartParser::reset() {
    mState        = State::SeekFirstBoundary;
    mBuffer.clear();
    mHeaders.clear();
    mBodyExpected = 0;
}

bool StreamingMultipartParser::push(const uint8_t* data, size_t length) {
    if (length == 0 || !data) return true;
    mBuffer.insert(mBuffer.end(), data, data + length);
    while (processBuffer()) {
        // keep processing until no more whole parts can be extracted
    }
    return true;
}

bool StreamingMultipartParser::processBuffer() {
    switch (mState) {
        case State::SeekFirstBoundary: {
            size_t pos = findBytes(mBuffer, 0, mDashBoundary);
            if (pos == std::string::npos) {
                // Keep last (mDashBoundary.size()-1) bytes in case the
                // boundary straddles two chunks.
                if (mBuffer.size() > mDashBoundary.size()) {
                    mBuffer.erase(
                        mBuffer.begin(),
                        mBuffer.end() - (mDashBoundary.size() - 1));
                }
                return false;
            }
            // Drop everything up to and including the boundary marker
            size_t consume = pos + mDashBoundary.size();
            // Optional CRLF after the boundary
            if (consume + 1 < mBuffer.size() &&
                mBuffer[consume] == '\r' && mBuffer[consume + 1] == '\n') {
                consume += 2;
            }
            mBuffer.erase(mBuffer.begin(), mBuffer.begin() + consume);
            mState        = State::ReadHeaders;
            mHeaders.clear();
            mBodyExpected = 0;
            return true;
        }

        case State::ReadHeaders: {
            size_t headerEnd = findBytes(mBuffer, 0, "\r\n\r\n");
            if (headerEnd == std::string::npos) {
                return false;
            }
            if (!parseHeadersFromBuffer(headerEnd)) {
                // Malformed -- reset
                reset();
                return false;
            }
            mBuffer.erase(mBuffer.begin(), mBuffer.begin() + headerEnd + 4);

            auto clen = headerValue(mHeaders, "Content-Length");
            mBodyExpected = clen.empty() ? 0
                                          : static_cast<size_t>(
                                                std::strtoul(clen.c_str(),
                                                              nullptr, 10));
            mState = State::ReadBody;
            return true;
        }

        case State::ReadBody: {
            if (mBodyExpected > 0) {
                if (mBuffer.size() < mBodyExpected) return false;
                std::vector<uint8_t> body(
                    mBuffer.begin(), mBuffer.begin() + mBodyExpected);
                mBuffer.erase(mBuffer.begin(),
                              mBuffer.begin() + mBodyExpected);
                emitPart(std::move(body));
                // After the body the server emits CRLF + next boundary.
                mState = State::SeekFirstBoundary;
                return true;
            }
            // Unknown length -- scan for the next boundary preceded by CRLF
            size_t pos = findBytes(mBuffer, 0, "\r\n" + mDashBoundary);
            if (pos == std::string::npos) {
                // Keep enough bytes to match the boundary on the next chunk
                size_t keep = mDashBoundary.size() + 2;
                if (mBuffer.size() > keep) {
                    // Cannot emit yet -- need more data
                }
                return false;
            }
            std::vector<uint8_t> body(mBuffer.begin(),
                                      mBuffer.begin() + pos);
            mBuffer.erase(mBuffer.begin(), mBuffer.begin() + pos + 2);
            emitPart(std::move(body));
            mState = State::SeekFirstBoundary;
            return true;
        }

        case State::Done:
            return false;
    }
    return false;
}

bool StreamingMultipartParser::parseHeadersFromBuffer(size_t headersEnd) {
    std::string blob(mBuffer.begin(), mBuffer.begin() + headersEnd);
    blob += "\r\n";
    mHeaders = parseHeaderLines(blob);
    return true;
}

void StreamingMultipartParser::emitPart(std::vector<uint8_t>&& body) {
    if (!mCallback) return;
    CgiImage part;
    part.mimeType = headerValue(mHeaders, "Content-Type");
    part.headers  = mHeaders;
    part.data     = std::move(body);
    mCallback(part);
}

}  // namespace detail
}  // namespace itscam
