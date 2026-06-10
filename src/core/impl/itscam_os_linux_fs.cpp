/*
 *  itscam_os_linux_fs.cpp
 *
 *  ITSCAM Client SDK - Linux/POSIX filesystem helpers
 *
 *  Copyright (c) 2026 Pumatronix
 */

#if !defined(_WIN32) && !defined(__MINGW32__)

#include "../itscam_os.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

namespace itscam {
namespace os {
namespace fs {

namespace {

constexpr char kSep = '/';

bool statRaw(const std::string& path, struct ::stat& st) {
    return ::stat(path.c_str(), &st) == 0;
}

bool fillFileInfo(const std::string& dir,
                  const std::string& name,
                  FileInfo& info) {
    std::string full = (dir.empty() || dir.back() == kSep)
        ? dir + name
        : dir + kSep + name;
    struct ::stat st{};
    if (::lstat(full.c_str(), &st) != 0) {
        return false;
    }
    info.name = name;
    info.absPath = realpath(full);
    if (info.absPath.empty()) {
        info.absPath = full;
    }
    info.isDirectory = S_ISDIR(st.st_mode);
    info.size = info.isDirectory ? 0u : static_cast<std::uint64_t>(st.st_size);
    info.mtimeMs = static_cast<std::int64_t>(st.st_mtim.tv_sec) * 1000
                 + static_cast<std::int64_t>(st.st_mtim.tv_nsec / 1000000);
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
    DIR* dir = ::opendir(path.c_str());
    if (!dir) {
        return false;
    }

    struct dirent* entry = nullptr;
    while ((entry = ::readdir(dir)) != nullptr) {
        const char* name = entry->d_name;
        if (name[0] == '.' && (name[1] == '\0' ||
                              (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }
        FileInfo info;
        if (fillFileInfo(path, name, info)) {
            fn(info);
            if (recursive && info.isDirectory) {
                walkImpl(info.absPath, true, fn);
            }
        }
    }

    ::closedir(dir);
    return true;
}

}  // namespace

bool exists(const std::string& path) {
    struct ::stat st{};
    return ::lstat(path.c_str(), &st) == 0;
}

bool isDir(const std::string& path) {
    struct ::stat st{};
    return statRaw(path, st) && S_ISDIR(st.st_mode);
}

bool isFile(const std::string& path) {
    struct ::stat st{};
    return statRaw(path, st) && S_ISREG(st.st_mode);
}

bool mkdir(const std::string& path) {
    if (::mkdir(path.c_str(), 0755) == 0) {
        return true;
    }
    return errno == EEXIST && isDir(path);
}

bool mkpath(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (isDir(path)) {
        return true;
    }

    std::string buf = path;
    // Strip trailing separators (but keep root "/")
    while (buf.size() > 1 && buf.back() == kSep) {
        buf.pop_back();
    }

    for (std::size_t i = 1; i < buf.size(); ++i) {
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
    return ::rename(from.c_str(), to.c_str()) == 0;
}

bool removeFile(const std::string& path) {
    return ::unlink(path.c_str()) == 0;
}

bool removeTree(const std::string& path, bool emptyOnly) {
    if (!isDir(path)) {
        return removeFile(path);
    }

    DIR* dir = ::opendir(path.c_str());
    if (!dir) {
        return false;
    }

    bool ok = true;
    struct dirent* entry = nullptr;
    while ((entry = ::readdir(dir)) != nullptr) {
        const char* name = entry->d_name;
        if (name[0] == '.' && (name[1] == '\0' ||
                              (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }
        std::string child = join(path, name);
        struct ::stat st{};
        if (::lstat(child.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            ok = removeTree(child, false) && ok;
        } else {
            ok = (::unlink(child.c_str()) == 0) && ok;
        }
    }
    ::closedir(dir);

    if (!emptyOnly) {
        ok = (::rmdir(path.c_str()) == 0) && ok;
    }
    return ok;
}

bool copyFile(const std::string& src, const std::string& dst) {
    int srcFd = ::open(src.c_str(), O_RDONLY);
    if (srcFd < 0) {
        return false;
    }
    struct ::stat st{};
    if (::fstat(srcFd, &st) != 0) {
        ::close(srcFd);
        return false;
    }
    int dstFd = ::open(dst.c_str(),
                       O_WRONLY | O_CREAT | O_TRUNC,
                       st.st_mode & 0777);
    if (dstFd < 0) {
        ::close(srcFd);
        return false;
    }

    constexpr std::size_t kBuf = 64 * 1024;
    char buf[kBuf];
    bool ok = true;
    while (true) {
        ssize_t n = ::read(srcFd, buf, kBuf);
        if (n < 0) {
            if (errno == EINTR) continue;
            ok = false;
            break;
        }
        if (n == 0) {
            break;
        }
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = ::write(dstFd, buf + written, n - written);
            if (w < 0) {
                if (errno == EINTR) continue;
                ok = false;
                break;
            }
            written += w;
        }
        if (!ok) break;
    }

    ::close(srcFd);
    ::close(dstFd);
    if (!ok) {
        ::unlink(dst.c_str());
    }
    return ok;
}

bool hardLink(const std::string& src, const std::string& dst) {
    return ::link(src.c_str(), dst.c_str()) == 0;
}

bool stat(const std::string& path, FileInfo& out) {
    struct ::stat st{};
    if (::lstat(path.c_str(), &st) != 0) {
        return false;
    }
    // Fully qualify -- the unqualified names below would resolve to the
    // POSIX ::basename / ::realpath declared in <libgen.h> / <stdlib.h>
    // (their string overloads are declared later in this TU).
    out.name = ::itscam::os::fs::basename(path);
    out.absPath = ::itscam::os::fs::realpath(path);
    if (out.absPath.empty()) {
        out.absPath = path;
    }
    out.isDirectory = S_ISDIR(st.st_mode);
    out.size = out.isDirectory ? 0u : static_cast<std::uint64_t>(st.st_size);
    out.mtimeMs = static_cast<std::int64_t>(st.st_mtim.tv_sec) * 1000
                + static_cast<std::int64_t>(st.st_mtim.tv_nsec / 1000000);
    return true;
}

std::string realpath(const std::string& path) {
    char* resolved = ::realpath(path.c_str(), nullptr);
    if (!resolved) {
        return {};
    }
    std::string out(resolved);
    ::free(resolved);
    return out;
}

std::string basename(const std::string& path) {
    // Trim trailing separators (but keep a lone "/" as-is).
    auto end = path.size();
    while (end > 1 && path[end - 1] == kSep) {
        --end;
    }
    if (end == 0) return std::string();
    if (end == 1 && path[0] == kSep) return std::string(1, kSep);

    auto slash = path.rfind(kSep, end - 1);
    if (slash == std::string::npos) {
        return path.substr(0, end);
    }
    return path.substr(slash + 1, end - slash - 1);
}

std::string dirname(const std::string& path) {
    auto end = path.size();
    while (end > 1 && path[end - 1] == kSep) {
        --end;
    }
    if (end == 0) return std::string(".");

    auto slash = path.rfind(kSep, end - 1);
    if (slash == std::string::npos) return std::string(".");
    if (slash == 0)                 return std::string(1, kSep);

    // Trim trailing slashes from the parent component too.
    while (slash > 0 && path[slash - 1] == kSep) {
        --slash;
    }
    return slash == 0 ? std::string(1, kSep) : path.substr(0, slash);
}

std::string join(const std::string& a, const std::string& b) {
    if (b.empty()) return a;
    if (a.empty()) return b;
    if (b.front() == kSep) return b;

    std::string out = a;
    if (out.back() != kSep) {
        out.push_back(kSep);
    }
    out.append(b);
    return out;
}

std::string cwd() {
    char buf[PATH_MAX];
    if (::getcwd(buf, sizeof(buf)) == nullptr) {
        return {};
    }
    return std::string(buf);
}

std::uint64_t availableSpace(const std::string& path) {
    struct statvfs vfs{};
    if (::statvfs(path.c_str(), &vfs) != 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(vfs.f_bavail) *
           static_cast<std::uint64_t>(vfs.f_frsize);
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

#endif  // !_WIN32
