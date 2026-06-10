/*
 *  itscam_os_windows_fs.cpp
 *
 *  ITSCAM Client SDK - Windows filesystem helpers
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  All public paths are UTF-8.  This file converts to UTF-16 (wide)
 *  internally so the wide Win32 APIs are used end-to-end, which keeps
 *  non-ASCII paths working regardless of the system code page.
 */

#if defined(_WIN32) || defined(__MINGW32__)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include "../itscam_os.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace itscam {
namespace os {
namespace fs {

namespace {

constexpr char     kSep   = '\\';
constexpr wchar_t  kSepW  = L'\\';

std::wstring utf8ToWide(const std::string& in) {
    if (in.empty()) return {};
    int len = ::MultiByteToWideChar(CP_UTF8, 0,
                                    in.data(), static_cast<int>(in.size()),
                                    nullptr, 0);
    if (len <= 0) return {};
    std::wstring out(static_cast<std::size_t>(len), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0,
                          in.data(), static_cast<int>(in.size()),
                          &out[0], len);
    return out;
}

std::string wideToUtf8(const wchar_t* in, std::size_t inLen) {
    if (inLen == 0) return {};
    int len = ::WideCharToMultiByte(CP_UTF8, 0,
                                    in, static_cast<int>(inLen),
                                    nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(static_cast<std::size_t>(len), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0,
                          in, static_cast<int>(inLen),
                          &out[0], len, nullptr, nullptr);
    return out;
}

std::string wideToUtf8(const std::wstring& in) {
    return wideToUtf8(in.data(), in.size());
}

std::int64_t fileTimeToEpochMs(const FILETIME& ft) {
    // FILETIME: 100-ns intervals since 1601-01-01.
    // Unix epoch is 1970-01-01.
    constexpr std::uint64_t kEpochDiff100ns = 116444736000000000ULL;
    std::uint64_t t = (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32)
                      | static_cast<std::uint64_t>(ft.dwLowDateTime);
    if (t < kEpochDiff100ns) {
        return 0;
    }
    return static_cast<std::int64_t>((t - kEpochDiff100ns) / 10000ULL);
}

bool isDotOrDotDot(const wchar_t* name) {
    return name[0] == L'.' &&
           (name[1] == L'\0' ||
            (name[1] == L'.' && name[2] == L'\0'));
}

void normalizeSep(std::string& path) {
    for (char& c : path) {
        if (c == '/') c = kSep;
    }
}

bool fillFileInfo(const std::wstring& parent,
                  const WIN32_FIND_DATAW& data,
                  FileInfo& info) {
    std::wstring full = parent;
    if (!full.empty() && full.back() != kSepW && full.back() != L'/') {
        full.push_back(kSepW);
    }
    full.append(data.cFileName);

    info.name = wideToUtf8(data.cFileName, std::wcslen(data.cFileName));
    info.absPath = realpath(wideToUtf8(full));
    if (info.absPath.empty()) {
        info.absPath = wideToUtf8(full);
    }
    info.isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    info.size = info.isDirectory
        ? 0u
        : ((static_cast<std::uint64_t>(data.nFileSizeHigh) << 32)
           | static_cast<std::uint64_t>(data.nFileSizeLow));
    info.mtimeMs = fileTimeToEpochMs(data.ftLastWriteTime);
    return true;
}

void applySort(std::vector<FileInfo>& entries, Sort sort) {
    switch (sort) {
        case Sort::None:
            return;
        case Sort::NameAsc:
            std::sort(entries.begin(), entries.end(),
                      [](const FileInfo& a, const FileInfo& b) {
                          return a.name < b.name;
                      });
            return;
        case Sort::NameDesc:
            std::sort(entries.begin(), entries.end(),
                      [](const FileInfo& a, const FileInfo& b) {
                          return a.name > b.name;
                      });
            return;
        case Sort::TimeAsc:
            std::sort(entries.begin(), entries.end(),
                      [](const FileInfo& a, const FileInfo& b) {
                          return a.mtimeMs < b.mtimeMs;
                      });
            return;
        case Sort::TimeDesc:
            std::sort(entries.begin(), entries.end(),
                      [](const FileInfo& a, const FileInfo& b) {
                          return a.mtimeMs > b.mtimeMs;
                      });
            return;
    }
}

bool walkImpl(const std::string& path,
              bool recursive,
              const std::function<void(const FileInfo&)>& fn) {
    std::wstring wpath = utf8ToWide(path);
    if (wpath.empty()) return false;

    std::wstring pattern = wpath;
    if (!pattern.empty() && pattern.back() != kSepW && pattern.back() != L'/') {
        pattern.push_back(kSepW);
    }
    pattern.append(L"*");

    WIN32_FIND_DATAW data{};
    HANDLE h = ::FindFirstFileW(pattern.c_str(), &data);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (isDotOrDotDot(data.cFileName)) {
            continue;
        }
        FileInfo info;
        fillFileInfo(wpath, data, info);
        fn(info);
        if (recursive && info.isDirectory) {
            walkImpl(info.absPath, true, fn);
        }
    } while (::FindNextFileW(h, &data));

    ::FindClose(h);
    return true;
}

}  // namespace

bool exists(const std::string& path) {
    std::wstring w = utf8ToWide(path);
    if (w.empty()) return false;
    return ::GetFileAttributesW(w.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool isDir(const std::string& path) {
    std::wstring w = utf8ToWide(path);
    if (w.empty()) return false;
    DWORD attr = ::GetFileAttributesW(w.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) &&
           (attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool isFile(const std::string& path) {
    std::wstring w = utf8ToWide(path);
    if (w.empty()) return false;
    DWORD attr = ::GetFileAttributesW(w.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) &&
           !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool mkdir(const std::string& path) {
    std::wstring w = utf8ToWide(path);
    if (w.empty()) return false;
    if (::CreateDirectoryW(w.c_str(), nullptr)) {
        return true;
    }
    return ::GetLastError() == ERROR_ALREADY_EXISTS && isDir(path);
}

bool mkpath(const std::string& path) {
    if (path.empty()) return false;
    if (isDir(path)) return true;

    std::string buf = path;
    normalizeSep(buf);
    while (buf.size() > 1 && buf.back() == kSep) {
        buf.pop_back();
    }

    // Skip drive prefix ("C:") if present.
    std::size_t start = 0;
    if (buf.size() >= 2 && buf[1] == ':') {
        start = 2;
    }
    // Skip UNC prefix ("\\server\share").
    if (buf.size() >= 2 && buf[0] == kSep && buf[1] == kSep) {
        std::size_t third = buf.find(kSep, 2);
        if (third != std::string::npos) {
            std::size_t fourth = buf.find(kSep, third + 1);
            start = (fourth == std::string::npos) ? buf.size() : fourth;
        }
    }

    for (std::size_t i = start + 1; i < buf.size(); ++i) {
        if (buf[i] == kSep) {
            std::string parent = buf.substr(0, i);
            if (!parent.empty() && !mkdir(parent)) {
                return false;
            }
        }
    }
    return mkdir(buf);
}

bool rename(const std::string& from, const std::string& to) {
    std::wstring wf = utf8ToWide(from);
    std::wstring wt = utf8ToWide(to);
    if (wf.empty() || wt.empty()) return false;
    return ::MoveFileExW(wf.c_str(), wt.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
}

bool removeFile(const std::string& path) {
    std::wstring w = utf8ToWide(path);
    if (w.empty()) return false;
    return ::DeleteFileW(w.c_str()) != 0;
}

bool removeTree(const std::string& path, bool emptyOnly) {
    if (!isDir(path)) {
        return removeFile(path);
    }

    std::wstring wpath = utf8ToWide(path);
    std::wstring pattern = wpath;
    if (!pattern.empty() && pattern.back() != kSepW && pattern.back() != L'/') {
        pattern.push_back(kSepW);
    }
    pattern.append(L"*");

    WIN32_FIND_DATAW data{};
    HANDLE h = ::FindFirstFileW(pattern.c_str(), &data);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool ok = true;
    do {
        if (isDotOrDotDot(data.cFileName)) {
            continue;
        }
        std::wstring child = wpath;
        if (!child.empty() && child.back() != kSepW && child.back() != L'/') {
            child.push_back(kSepW);
        }
        child.append(data.cFileName);
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ok = removeTree(wideToUtf8(child), false) && ok;
        } else {
            ok = (::DeleteFileW(child.c_str()) != 0) && ok;
        }
    } while (::FindNextFileW(h, &data));

    ::FindClose(h);

    if (!emptyOnly) {
        ok = (::RemoveDirectoryW(wpath.c_str()) != 0) && ok;
    }
    return ok;
}

bool copyFile(const std::string& src, const std::string& dst) {
    std::wstring ws = utf8ToWide(src);
    std::wstring wd = utf8ToWide(dst);
    if (ws.empty() || wd.empty()) return false;
    // FALSE => overwrite existing.
    return ::CopyFileW(ws.c_str(), wd.c_str(), FALSE) != 0;
}

bool hardLink(const std::string& src, const std::string& dst) {
    std::wstring ws = utf8ToWide(src);
    std::wstring wd = utf8ToWide(dst);
    if (ws.empty() || wd.empty()) return false;
    return ::CreateHardLinkW(wd.c_str(), ws.c_str(), nullptr) != 0;
}

bool stat(const std::string& path, FileInfo& out) {
    std::wstring w = utf8ToWide(path);
    if (w.empty()) return false;

    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!::GetFileAttributesExW(w.c_str(), GetFileExInfoStandard, &data)) {
        return false;
    }
    out.name = basename(path);
    out.absPath = realpath(path);
    if (out.absPath.empty()) {
        out.absPath = path;
    }
    out.isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    out.size = out.isDirectory
        ? 0u
        : ((static_cast<std::uint64_t>(data.nFileSizeHigh) << 32)
           | static_cast<std::uint64_t>(data.nFileSizeLow));
    out.mtimeMs = fileTimeToEpochMs(data.ftLastWriteTime);
    return true;
}

std::string realpath(const std::string& path) {
    std::wstring w = utf8ToWide(path);
    if (w.empty()) return {};

    DWORD needed = ::GetFullPathNameW(w.c_str(), 0, nullptr, nullptr);
    if (needed == 0) return {};
    std::wstring buf(static_cast<std::size_t>(needed), L'\0');
    DWORD wrote = ::GetFullPathNameW(w.c_str(), needed, &buf[0], nullptr);
    if (wrote == 0 || wrote >= needed) return {};
    buf.resize(wrote);
    return wideToUtf8(buf);
}

std::string basename(const std::string& path) {
    std::string norm = path;
    normalizeSep(norm);
    while (norm.size() > 1 && norm.back() == kSep) {
        norm.pop_back();
    }
    std::size_t pos = norm.find_last_of(kSep);
    if (pos == std::string::npos) return norm;
    return norm.substr(pos + 1);
}

std::string dirname(const std::string& path) {
    std::string norm = path;
    normalizeSep(norm);
    while (norm.size() > 1 && norm.back() == kSep) {
        norm.pop_back();
    }
    std::size_t pos = norm.find_last_of(kSep);
    if (pos == std::string::npos) return std::string(".");
    if (pos == 0) return std::string(1, kSep);
    // Preserve drive-letter root ("C:\")
    if (pos == 2 && norm.size() >= 3 && norm[1] == ':') {
        return norm.substr(0, 3);
    }
    return norm.substr(0, pos);
}

std::string join(const std::string& a, const std::string& b) {
    if (b.empty()) return a;
    if (a.empty()) return b;
    // Absolute on Windows: starts with separator or has "X:" prefix.
    if (b.front() == '/' || b.front() == '\\') return b;
    if (b.size() >= 2 && b[1] == ':') return b;

    std::string out = a;
    char last = out.back();
    if (last != '/' && last != '\\') {
        out.push_back(kSep);
    }
    out.append(b);
    return out;
}

std::string cwd() {
    DWORD needed = ::GetCurrentDirectoryW(0, nullptr);
    if (needed == 0) return {};
    std::wstring buf(static_cast<std::size_t>(needed), L'\0');
    DWORD wrote = ::GetCurrentDirectoryW(needed, &buf[0]);
    if (wrote == 0 || wrote >= needed) return {};
    buf.resize(wrote);
    return wideToUtf8(buf);
}

std::uint64_t availableSpace(const std::string& path) {
    std::wstring w = utf8ToWide(path);
    if (w.empty()) return 0;
    ULARGE_INTEGER free{};
    ULARGE_INTEGER total{};
    ULARGE_INTEGER totalFree{};
    if (!::GetDiskFreeSpaceExW(w.c_str(), &free, &total, &totalFree)) {
        return 0;
    }
    return static_cast<std::uint64_t>(free.QuadPart);
}

std::vector<FileInfo> listDir(const std::string& path,
                              bool recursive,
                              Sort sort) {
    std::vector<FileInfo> entries;
    walkImpl(path, recursive, [&entries](const FileInfo& info) {
        entries.push_back(info);
    });
    applySort(entries, sort);
    return entries;
}

std::vector<FileInfo> listDir(const std::string& path,
                              const std::string& suffix,
                              bool recursive,
                              Sort sort) {
    std::vector<FileInfo> entries;
    walkImpl(path, recursive, [&entries, &suffix](const FileInfo& info) {
        if (suffix.empty() ||
            (info.name.size() >= suffix.size() &&
             info.name.compare(info.name.size() - suffix.size(),
                               suffix.size(), suffix) == 0)) {
            entries.push_back(info);
        }
    });
    applySort(entries, sort);
    return entries;
}

bool walk(const std::string& path,
          bool recursive,
          const std::function<void(const FileInfo&)>& fn) {
    return walkImpl(path, recursive, fn);
}

}  // namespace fs
}  // namespace os
}  // namespace itscam

#endif  // _WIN32
