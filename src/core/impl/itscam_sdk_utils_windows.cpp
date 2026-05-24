/*
 *  itscam_sdk_utils_windows.cpp
 *
 *  ITSCAM Client SDK - Windows implementation of utility functions
 *
 *  Copyright (c) 2026 Pumatronix
 */

#ifndef _WIN32
#error "This file should only be compiled on Windows"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// Require Windows 10 or later
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include "../itscam_sdk_utils.h"

#include <windows.h>
#include <cstdio>
#include <cstring>

namespace itscam_utils {

// Thread-local error buffer
static thread_local char s_errorBuffer[256] = {0};

static void setError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_errorBuffer, sizeof(s_errorBuffer), fmt, args);
    va_end(args);
}

static void setErrorFromWin32(const char* prefix) {
    DWORD err = GetLastError();
    char buf[128];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, 0, buf, sizeof(buf), nullptr);
    // Remove trailing newline
    size_t len = strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
        buf[--len] = '\0';
    }
    snprintf(s_errorBuffer, sizeof(s_errorBuffer), "%s: %s", prefix, buf);
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
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    ts.year        = st.wYear;
    ts.month       = st.wMonth;
    ts.day         = st.wDay;
    ts.hour        = st.wHour;
    ts.minute      = st.wMinute;
    ts.second      = st.wSecond;
    ts.millisecond = st.wMilliseconds;
    
    // Get timezone offset
    TIME_ZONE_INFORMATION tzi;
    DWORD tzResult = GetTimeZoneInformation(&tzi);
    int32_t bias = tzi.Bias;  // In minutes, positive = west of UTC
    if (tzResult == TIME_ZONE_ID_DAYLIGHT) {
        bias += tzi.DaylightBias;
    } else if (tzResult == TIME_ZONE_ID_STANDARD) {
        bias += tzi.StandardBias;
    }
    ts.timezone_offset = -bias * 60;  // Convert to seconds, flip sign
    
    return ts;
}

Timestamp getSystemUtcTime() {
    Timestamp ts = {};
    
    SYSTEMTIME st;
    GetSystemTime(&st);
    
    ts.year        = st.wYear;
    ts.month       = st.wMonth;
    ts.day         = st.wDay;
    ts.hour        = st.wHour;
    ts.minute      = st.wMinute;
    ts.second      = st.wSecond;
    ts.millisecond = st.wMilliseconds;
    ts.timezone_offset = 0;
    
    return ts;
}

uint64_t getEpochTime() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    
    // Convert from 100-nanosecond intervals since 1601 to seconds since 1970
    // Difference: 11644473600 seconds
    return (uli.QuadPart / 10000000ULL) - 11644473600ULL;
}

uint64_t getEpochTimeMs() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    
    // Convert from 100-nanosecond intervals since 1601 to milliseconds since 1970
    return (uli.QuadPart / 10000ULL) - 11644473600000ULL;
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
    
    HANDLE hFile = CreateFileA(
        path,
        GENERIC_WRITE,
        0,  // No sharing
        nullptr,
        overwrite ? CREATE_ALWAYS : CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        setErrorFromWin32("Failed to create file");
        return false;
    }
    
    DWORD written = 0;
    BOOL success = WriteFile(hFile, data, static_cast<DWORD>(size), &written, nullptr);
    CloseHandle(hFile);
    
    if (!success || written != size) {
        setError("Failed to write all data: %lu of %zu bytes", written, size);
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
    
    HANDLE hFile = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        setErrorFromWin32("Failed to open file");
        return false;
    }
    
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        setErrorFromWin32("Failed to get file size");
        return false;
    }
    
    out.resize(static_cast<size_t>(fileSize.QuadPart));
    
    DWORD read = 0;
    BOOL success = ReadFile(hFile, out.data(), static_cast<DWORD>(out.size()), &read, nullptr);
    CloseHandle(hFile);
    
    if (!success || read != out.size()) {
        setError("Failed to read all data: %lu of %zu bytes", read, out.size());
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
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && 
           !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

uint64_t getFileSize(const char* path) {
    if (!path) return 0;
    
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fad)) {
        return 0;
    }
    
    ULARGE_INTEGER size;
    size.LowPart = fad.nFileSizeLow;
    size.HighPart = fad.nFileSizeHigh;
    return size.QuadPart;
}

bool deleteFile(const char* path) {
    if (!path) {
        setError("Invalid parameter: null pointer");
        return false;
    }
    if (!DeleteFileA(path)) {
        setErrorFromWin32("Failed to delete file");
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
    DWORD attrs = GetFileAttributesA(path);
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
            return true;  // Already exists
        }
        setError("Path exists but is not a directory: %s", path);
        return false;
    }
    
    if (recursive) {
        // Create parent directories first
        std::string pathStr(path);
        
        // Handle both / and \ as separators
        for (size_t i = 0; i < pathStr.size(); ++i) {
            if (pathStr[i] == '/' || pathStr[i] == '\\') {
                if (i > 0 && pathStr[i-1] != ':') {  // Skip drive letter
                    std::string subPath = pathStr.substr(0, i);
                    DWORD subAttrs = GetFileAttributesA(subPath.c_str());
                    if (subAttrs == INVALID_FILE_ATTRIBUTES) {
                        if (!CreateDirectoryA(subPath.c_str(), nullptr)) {
                            DWORD err = GetLastError();
                            if (err != ERROR_ALREADY_EXISTS) {
                                setErrorFromWin32("Failed to create directory");
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Create the final directory
    if (!CreateDirectoryA(path, nullptr)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
            setErrorFromWin32("Failed to create directory");
            return false;
        }
    }
    
    return true;
}

bool folderExists(const char* path) {
    if (!path) return false;
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && 
           (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool deleteFolder(const char* path, bool recursive) {
    if (!path) {
        setError("Invalid parameter: null pointer");
        return false;
    }
    
    if (recursive) {
        WIN32_FIND_DATAA findData;
        std::string searchPath = std::string(path) + "\\*";
        
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) {
            setErrorFromWin32("Failed to open directory");
            return false;
        }
        
        do {
            if (strcmp(findData.cFileName, ".") == 0 || 
                strcmp(findData.cFileName, "..") == 0) {
                continue;
            }
            
            std::string fullPath = joinPath(path, findData.cFileName);
            
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!deleteFolder(fullPath.c_str(), true)) {
                    FindClose(hFind);
                    return false;
                }
            } else {
                if (!DeleteFileA(fullPath.c_str())) {
                    FindClose(hFind);
                    setErrorFromWin32("Failed to delete file");
                    return false;
                }
            }
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
    }
    
    if (!RemoveDirectoryA(path)) {
        setErrorFromWin32("Failed to delete directory");
        return false;
    }
    
    return true;
}

std::vector<std::string> listFiles(const char* path, bool recursive, 
                                    const char* extension) {
    std::vector<std::string> result;
    
    if (!path) return result;
    
    WIN32_FIND_DATAA findData;
    std::string searchPath = std::string(path) + "\\*";
    
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return result;
    }
    
    std::string extFilter = extension ? extension : "";
    
    do {
        if (strcmp(findData.cFileName, ".") == 0 || 
            strcmp(findData.cFileName, "..") == 0) {
            continue;
        }
        
        std::string fullPath = joinPath(path, findData.cFileName);
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                auto subFiles = listFiles(fullPath.c_str(), true, extension);
                result.insert(result.end(), subFiles.begin(), subFiles.end());
            }
        } else {
            if (extFilter.empty() || getExtension(fullPath) == extFilter) {
                result.push_back(fullPath);
            }
        }
    } while (FindNextFileA(hFind, &findData));
    
    FindClose(hFind);
    return result;
}

//=========================================================================
// Path Utilities
//=========================================================================

std::string joinPath(const std::string& base, const std::string& name) {
    if (base.empty()) return name;
    if (name.empty()) return base;
    
    char last = base.back();
    if (last == '/' || last == '\\') {
        return base + name;
    }
    return base + '\\' + name;
}

std::string getDirectory(const std::string& path) {
    size_t posSlash = path.rfind('/');
    size_t posBackslash = path.rfind('\\');
    size_t pos = std::string::npos;
    
    if (posSlash != std::string::npos && posBackslash != std::string::npos) {
        pos = (posSlash > posBackslash) ? posSlash : posBackslash;
    } else if (posSlash != std::string::npos) {
        pos = posSlash;
    } else if (posBackslash != std::string::npos) {
        pos = posBackslash;
    }
    
    if (pos == std::string::npos) {
        return "";
    }
    return path.substr(0, pos);
}

std::string getFilename(const std::string& path) {
    size_t posSlash = path.rfind('/');
    size_t posBackslash = path.rfind('\\');
    size_t pos = std::string::npos;
    
    if (posSlash != std::string::npos && posBackslash != std::string::npos) {
        pos = (posSlash > posBackslash) ? posSlash : posBackslash;
    } else if (posSlash != std::string::npos) {
        pos = posSlash;
    } else if (posBackslash != std::string::npos) {
        pos = posBackslash;
    }
    
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
    return '\\';
}

//=========================================================================
// Error Handling
//=========================================================================

const char* getLastUtilError() {
    return s_errorBuffer;
}

} // namespace itscam_utils
