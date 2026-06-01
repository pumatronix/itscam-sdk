/*
 *  itscam_sdk_utils.h
 *
 *  ITSCAM Client SDK - Cross-platform utility functions
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Platform-independent API for file I/O, directory operations, and time.
 *  Supports Windows and Linux.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

//=========================================================================
// Export macro
//=========================================================================

#if defined(__GNUC__) && defined(__linux__)
#   define ITSCAM_UTILS_API __attribute__((visibility("default")))
#elif defined(_WIN32) || defined(__MINGW32__)
#   ifdef ITSCAM_SDK_BUILDING
#       define ITSCAM_UTILS_API __declspec(dllexport)
#   else
#       define ITSCAM_UTILS_API __declspec(dllimport)
#   endif
#else
#   define ITSCAM_UTILS_API
#endif

// Separate namespace to avoid conflicts with itscam::Timestamp in itscam_types.h
namespace itscam {
namespace utils {

//=========================================================================
// Timestamp
//=========================================================================

/**
 * @brief Timestamp structure representing a point in time.
 */
struct Timestamp {
    uint16_t year;              ///< Full year (e.g., 2026)
    uint16_t month;             ///< Month (1-12)
    uint16_t day;               ///< Day of month (1-31)
    uint16_t hour;              ///< Hour (0-23)
    uint16_t minute;            ///< Minute (0-59)
    uint16_t second;            ///< Second (0-59)
    uint16_t millisecond;       ///< Millisecond (0-999)
    int32_t  timezone_offset;   ///< Timezone offset from UTC in seconds

    /**
     * @brief Format timestamp as ISO 8601 string.
     * @return String in format "YYYY-MM-DDTHH:MM:SS.sss"
     */
    ITSCAM_UTILS_API std::string toIso8601() const;

    /**
     * @brief Format timestamp as compact string.
     * @return String in format "YYYYMMDD_HHMMSS"
     */
    ITSCAM_UTILS_API std::string toCompact() const;
};

//=========================================================================
// Time Functions
//=========================================================================

/**
 * @brief Get current system local time.
 * @return Timestamp in local timezone.
 */
ITSCAM_UTILS_API Timestamp getSystemLocalTime();

/**
 * @brief Get current system UTC time.
 * @return Timestamp in UTC.
 */
ITSCAM_UTILS_API Timestamp getSystemUtcTime();

/**
 * @brief Get Unix epoch time in seconds.
 * @return Seconds since January 1, 1970.
 */
ITSCAM_UTILS_API uint64_t getEpochTime();

/**
 * @brief Get Unix epoch time in milliseconds.
 * @return Milliseconds since January 1, 1970.
 */
ITSCAM_UTILS_API uint64_t getEpochTimeMs();

//=========================================================================
// File Operations
//=========================================================================

/**
 * @brief Store data to a file, creating parent directories if needed.
 * @param path      Full path to the file.
 * @param data      Pointer to data buffer.
 * @param size      Size of data in bytes.
 * @param overwrite If true, overwrite existing file; if false, fail if exists.
 * @return true on success, false on failure.
 */
ITSCAM_UTILS_API bool storeFile(const char* path, const uint8_t* data, size_t size,
                          bool overwrite = true);

/**
 * @brief Store string data to a file.
 * @param path      Full path to the file.
 * @param content   String content to write.
 * @param overwrite If true, overwrite existing file.
 * @return true on success, false on failure.
 */
ITSCAM_UTILS_API bool storeFile(const char* path, const std::string& content,
                          bool overwrite = true);

/**
 * @brief Store vector data to a file.
 * @param path      Full path to the file.
 * @param data      Vector of bytes to write.
 * @param overwrite If true, overwrite existing file.
 * @return true on success, false on failure.
 */
ITSCAM_UTILS_API bool storeFile(const char* path, const std::vector<uint8_t>& data,
                          bool overwrite = true);

/**
 * @brief Read entire file contents into a vector.
 * @param path Full path to the file.
 * @param out  Output vector (cleared before reading).
 * @return true on success, false on failure.
 */
ITSCAM_UTILS_API bool readFile(const char* path, std::vector<uint8_t>& out);

/**
 * @brief Read entire file contents into a string.
 * @param path Full path to the file.
 * @param out  Output string (cleared before reading).
 * @return true on success, false on failure.
 */
ITSCAM_UTILS_API bool readFile(const char* path, std::string& out);

/**
 * @brief Check if a file exists.
 * @param path Full path to check.
 * @return true if file exists and is accessible.
 */
ITSCAM_UTILS_API bool fileExists(const char* path);

/**
 * @brief Get the size of a file in bytes.
 * @param path Full path to the file.
 * @return File size in bytes, or 0 if file doesn't exist.
 */
ITSCAM_UTILS_API uint64_t getFileSize(const char* path);

/**
 * @brief Delete a file.
 * @param path Full path to the file.
 * @return true on success, false on failure.
 */
ITSCAM_UTILS_API bool deleteFile(const char* path);

//=========================================================================
// Directory Operations
//=========================================================================

/**
 * @brief Create a directory.
 * @param path      Full path to the directory.
 * @param recursive If true, create parent directories as needed (mkdir -p).
 * @return true on success (or directory already exists), false on failure.
 */
ITSCAM_UTILS_API bool createFolder(const char* path, bool recursive = true);

/**
 * @brief Check if a directory exists.
 * @param path Full path to check.
 * @return true if directory exists and is accessible.
 */
ITSCAM_UTILS_API bool folderExists(const char* path);

/**
 * @brief Delete a directory.
 * @param path      Full path to the directory.
 * @param recursive If true, delete contents recursively.
 * @return true on success, false on failure.
 */
ITSCAM_UTILS_API bool deleteFolder(const char* path, bool recursive = false);

/**
 * @brief List files in a directory.
 * @param path      Full path to the directory.
 * @param recursive If true, include files from subdirectories.
 * @param extension Filter by file extension (e.g., ".jpg"), empty for all.
 * @return Vector of file paths.
 */
ITSCAM_UTILS_API std::vector<std::string> listFiles(const char* path,
                                               bool recursive = false,
                                               const char* extension = "");

//=========================================================================
// Path Utilities
//=========================================================================

/**
 * @brief Join two path components.
 * @param base Base path.
 * @param name File or directory name to append.
 * @return Combined path with appropriate separator.
 */
ITSCAM_UTILS_API std::string joinPath(const std::string& base, const std::string& name);

/**
 * @brief Get the directory part of a path.
 * @param path Full path.
 * @return Directory component (without trailing separator).
 */
ITSCAM_UTILS_API std::string getDirectory(const std::string& path);

/**
 * @brief Get the filename part of a path.
 * @param path Full path.
 * @return Filename with extension.
 */
ITSCAM_UTILS_API std::string getFilename(const std::string& path);

/**
 * @brief Get the file extension.
 * @param path Full path or filename.
 * @return Extension including the dot (e.g., ".jpg"), or empty string.
 */
ITSCAM_UTILS_API std::string getExtension(const std::string& path);

/**
 * @brief Get the platform-specific path separator.
 * @return '/' on Linux, '\\' on Windows.
 */
ITSCAM_UTILS_API char getPathSeparator();

//=========================================================================
// Error Handling
//=========================================================================

/**
 * @brief Get the last error message from utility operations.
 * @return Error message string (thread-local).
 */
ITSCAM_UTILS_API const char* getLastUtilError();

} // namespace utils
} // namespace itscam
