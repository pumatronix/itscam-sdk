/*
 *  itscam_sdk_utils_linux.cpp
 *
 *  ITSCAM Client SDK - Linux implementation of utility functions
 *
 *  Copyright (c) 2026 Pumatronix
 */

#include "../itscam_sdk_utils.h"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

namespace itscam {
namespace utils {

// Thread-local error buffer
static thread_local char s_errorBuffer[256] = {0};

static void setError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_errorBuffer, sizeof(s_errorBuffer), fmt, args);
    va_end(args);
}

//=========================================================================
// Timestamp Methods
//=========================================================================

std::string Timestamp::toIso8601() const {
    char buf[48];
    snprintf(buf, sizeof(buf), "%04u-%02u-%02uT%02u:%02u:%02u.%03u",
             static_cast<unsigned>(year), static_cast<unsigned>(month),
             static_cast<unsigned>(day), static_cast<unsigned>(hour),
             static_cast<unsigned>(minute), static_cast<unsigned>(second),
             static_cast<unsigned>(millisecond));
    return buf;
}

std::string Timestamp::toCompact() const {
    char buf[32];
    snprintf(buf, sizeof(buf), "%04u%02u%02u_%02u%02u%02u",
             static_cast<unsigned>(year), static_cast<unsigned>(month),
             static_cast<unsigned>(day), static_cast<unsigned>(hour),
             static_cast<unsigned>(minute), static_cast<unsigned>(second));
    return buf;
}

//=========================================================================
// Time Functions
//=========================================================================

Timestamp getSystemLocalTime() {
    Timestamp ts = {};
    
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    
    struct tm tm_local;
    localtime_r(&tv.tv_sec, &tm_local);
    
    ts.year        = static_cast<uint16_t>(tm_local.tm_year + 1900);
    ts.month       = static_cast<uint16_t>(tm_local.tm_mon + 1);
    ts.day         = static_cast<uint16_t>(tm_local.tm_mday);
    ts.hour        = static_cast<uint16_t>(tm_local.tm_hour);
    ts.minute      = static_cast<uint16_t>(tm_local.tm_min);
    ts.second      = static_cast<uint16_t>(tm_local.tm_sec);
    ts.millisecond = static_cast<uint16_t>(tv.tv_usec / 1000);
    ts.timezone_offset = tm_local.tm_gmtoff;
    
    return ts;
}

Timestamp getSystemUtcTime() {
    Timestamp ts = {};
    
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    
    struct tm tm_utc;
    gmtime_r(&tv.tv_sec, &tm_utc);
    
    ts.year        = static_cast<uint16_t>(tm_utc.tm_year + 1900);
    ts.month       = static_cast<uint16_t>(tm_utc.tm_mon + 1);
    ts.day         = static_cast<uint16_t>(tm_utc.tm_mday);
    ts.hour        = static_cast<uint16_t>(tm_utc.tm_hour);
    ts.minute      = static_cast<uint16_t>(tm_utc.tm_min);
    ts.second      = static_cast<uint16_t>(tm_utc.tm_sec);
    ts.millisecond = static_cast<uint16_t>(tv.tv_usec / 1000);
    ts.timezone_offset = 0;
    
    return ts;
}

uint64_t getEpochTime() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec);
}

uint64_t getEpochTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * 1000 + 
           static_cast<uint64_t>(tv.tv_usec / 1000);
}

//=========================================================================
// File Operations
//=========================================================================

bool storeFile(const char* path, const uint8_t* data, size_t size, bool overwrite) {
    if (!path || !data) {
        setError("Invalid parameter: null pointer");
        return false;
    }
    
    // Create parent directories if needed
    std::string dir = getDirectory(path);
    if (!dir.empty() && !folderExists(dir.c_str())) {
        if (!createFolder(dir.c_str(), true)) {
            return false;  // Error already set
        }
    }
    
    // Check if file exists
    if (!overwrite && fileExists(path)) {
        setError("File already exists: %s", path);
        return false;
    }
    
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        setError("Failed to open file for writing: %s (%s)", path, strerror(errno));
        return false;
    }
    
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    
    if (written != size) {
        setError("Failed to write all data: %zu of %zu bytes", written, size);
        return false;
    }
    
    return true;
}

bool storeFile(const char* path, const std::string& content, bool overwrite) {
    return storeFile(path, reinterpret_cast<const uint8_t*>(content.data()),
                     content.size(), overwrite);
}

bool storeFile(const char* path, const std::vector<uint8_t>& data, bool overwrite) {
    return storeFile(path, data.data(), data.size(), overwrite);
}

bool readFile(const char* path, std::vector<uint8_t>& out) {
    out.clear();
    
    if (!path) {
        setError("Invalid parameter: null pointer");
        return false;
    }
    
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        setError("Failed to open file for reading: %s (%s)", path, strerror(errno));
        return false;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (size < 0) {
        fclose(fp);
        setError("Failed to get file size: %s", path);
        return false;
    }
    
    out.resize(static_cast<size_t>(size));
    size_t read = fread(out.data(), 1, out.size(), fp);
    fclose(fp);
    
    if (read != out.size()) {
        setError("Failed to read all data: %zu of %zu bytes", read, out.size());
        out.clear();
        return false;
    }
    
    return true;
}

bool readFile(const char* path, std::string& out) {
    std::vector<uint8_t> data;
    if (!readFile(path, data)) {
        out.clear();
        return false;
    }
    out.assign(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

bool fileExists(const char* path) {
    if (!path) return false;
    struct stat st;
    return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
}

uint64_t getFileSize(const char* path) {
    if (!path) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return static_cast<uint64_t>(st.st_size);
}

bool deleteFile(const char* path) {
    if (!path) {
        setError("Invalid parameter: null pointer");
        return false;
    }
    if (unlink(path) != 0) {
        setError("Failed to delete file: %s (%s)", path, strerror(errno));
        return false;
    }
    return true;
}

//=========================================================================
// Directory Operations
//=========================================================================

bool createFolder(const char* path, bool recursive) {
    if (!path || path[0] == '\0') {
        setError("Invalid parameter: empty path");
        return false;
    }
    
    // Check if already exists
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return true;  // Already exists
        }
        setError("Path exists but is not a directory: %s", path);
        return false;
    }
    
    if (recursive) {
        // Create parent directories first
        std::string pathStr(path);
        size_t pos = 0;
        
        while ((pos = pathStr.find('/', pos + 1)) != std::string::npos) {
            std::string subPath = pathStr.substr(0, pos);
            if (!subPath.empty() && subPath != "/") {
                if (stat(subPath.c_str(), &st) != 0) {
                    if (mkdir(subPath.c_str(), 0755) != 0 && errno != EEXIST) {
                        setError("Failed to create directory: %s (%s)", 
                                 subPath.c_str(), strerror(errno));
                        return false;
                    }
                }
            }
        }
    }
    
    // Create the final directory
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        setError("Failed to create directory: %s (%s)", path, strerror(errno));
        return false;
    }
    
    return true;
}

bool folderExists(const char* path) {
    if (!path) return false;
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

bool deleteFolder(const char* path, bool recursive) {
    if (!path) {
        setError("Invalid parameter: null pointer");
        return false;
    }
    
    if (recursive) {
        DIR* dir = opendir(path);
        if (!dir) {
            setError("Failed to open directory: %s (%s)", path, strerror(errno));
            return false;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            std::string fullPath = joinPath(path, entry->d_name);
            struct stat st;
            if (stat(fullPath.c_str(), &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    if (!deleteFolder(fullPath.c_str(), true)) {
                        closedir(dir);
                        return false;
                    }
                } else {
                    if (unlink(fullPath.c_str()) != 0) {
                        closedir(dir);
                        setError("Failed to delete file: %s (%s)", 
                                 fullPath.c_str(), strerror(errno));
                        return false;
                    }
                }
            }
        }
        closedir(dir);
    }
    
    if (rmdir(path) != 0) {
        setError("Failed to delete directory: %s (%s)", path, strerror(errno));
        return false;
    }
    
    return true;
}

std::vector<std::string> listFiles(const char* path, bool recursive, 
                                    const char* extension) {
    std::vector<std::string> result;
    
    if (!path) return result;
    
    DIR* dir = opendir(path);
    if (!dir) return result;
    
    std::string extFilter = extension ? extension : "";
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        std::string fullPath = joinPath(path, entry->d_name);
        struct stat st;
        if (stat(fullPath.c_str(), &st) != 0) {
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            if (recursive) {
                auto subFiles = listFiles(fullPath.c_str(), true, extension);
                result.insert(result.end(), subFiles.begin(), subFiles.end());
            }
        } else if (S_ISREG(st.st_mode)) {
            if (extFilter.empty() || getExtension(fullPath) == extFilter) {
                result.push_back(fullPath);
            }
        }
    }
    
    closedir(dir);
    return result;
}

//=========================================================================
// Path Utilities
//=========================================================================

std::string joinPath(const std::string& base, const std::string& name) {
    if (base.empty()) return name;
    if (name.empty()) return base;
    
    if (base.back() == '/') {
        return base + name;
    }
    return base + '/' + name;
}

std::string getDirectory(const std::string& path) {
    size_t pos = path.rfind('/');
    if (pos == std::string::npos) {
        return "";
    }
    if (pos == 0) {
        return "/";
    }
    return path.substr(0, pos);
}

std::string getFilename(const std::string& path) {
    size_t pos = path.rfind('/');
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

std::string getExtension(const std::string& path) {
    std::string filename = getFilename(path);
    size_t pos = filename.rfind('.');
    if (pos == std::string::npos || pos == 0) {
        return "";
    }
    return filename.substr(pos);
}

char getPathSeparator() {
    return '/';
}

//=========================================================================
// Error Handling
//=========================================================================

const char* getLastUtilError() {
    return s_errorBuffer;
}

} // namespace utils
} // namespace itscam
