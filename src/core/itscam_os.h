/*
 *  itscam_os.h
 *
 *  ITSCAM Client SDK - OS Abstraction Layer
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Cross-platform abstractions for:
 *  - Sockets (TCP/IP)
 *  - Threads
 *  - Mutexes
 *  - Condition variables
 *  - Time functions
 *
 *  Linux: Uses POSIX threads and BSD sockets
 *  Windows: Uses Win32 API and Winsock2
 */
#ifndef ITSCAM_OS_H
#define ITSCAM_OS_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>

// ============================================================================
//  Platform Detection
// ============================================================================

#if defined(_WIN32) || defined(__MINGW32__)
#   define ITSCAM_OS_WINDOWS 1
#   define ITSCAM_OS_LINUX   0
#else
#   define ITSCAM_OS_WINDOWS 0
#   define ITSCAM_OS_LINUX   1
#endif

// ============================================================================
//  Socket Error Codes (platform-independent)
// ============================================================================

#define ITSCAM_SOCK_OK            0
#define ITSCAM_SOCK_EWOULDBLOCK   1
#define ITSCAM_SOCK_EINPROGRESS   2
#define ITSCAM_SOCK_ECONNREFUSED  3
#define ITSCAM_SOCK_ETIMEDOUT     4
#define ITSCAM_SOCK_ENETUNREACH   5
#define ITSCAM_SOCK_ECONNRESET    6
#define ITSCAM_SOCK_EPIPE         7
#define ITSCAM_SOCK_EINTR         8
#define ITSCAM_SOCK_EUNKNOWN      99

// ============================================================================
//  OS Abstraction Namespace - Internal API
// ============================================================================

namespace itscam_os {

// ============================================================================
//  Platform-specific Types
// ============================================================================

#if ITSCAM_OS_WINDOWS
    // Forward declare Windows types to avoid including windows.h in header
    struct ThreadImpl;
    struct MutexImpl;
    struct CondImpl;
    using SocketHandle = void*;  // SOCKET is actually a pointer-sized handle
    // INVALID_SOCKET is (SOCKET)(~0) on Windows
    inline SocketHandle invalidSocket() { return reinterpret_cast<SocketHandle>(~static_cast<std::uintptr_t>(0)); }
    #define ITSCAM_OS_INVALID_SOCKET (itscam_os::invalidSocket())
#else
    // POSIX types
    struct ThreadImpl;
    struct MutexImpl;
    struct CondImpl;
    using SocketHandle = int;
    constexpr SocketHandle kInvalidSocket = -1;
    #define ITSCAM_OS_INVALID_SOCKET (itscam_os::kInvalidSocket)
#endif

// Opaque handles for thread primitives
using ThreadHandle = ThreadImpl*;
using MutexHandle = MutexImpl*;
using CondHandle = CondImpl*;

// Thread function signature
using ThreadFunc = void (*)(void* arg);

// ============================================================================
//  Byte Order Functions (Network <-> Host)
// ============================================================================

/** @brief Convert 16-bit value from host to network byte order. */
uint16_t hostToNet16(uint16_t hostshort);

/** @brief Convert 32-bit value from host to network byte order. */
uint32_t hostToNet32(uint32_t hostlong);

/** @brief Convert 16-bit value from network to host byte order. */
uint16_t netToHost16(uint16_t netshort);

/** @brief Convert 32-bit value from network to host byte order. */
uint32_t netToHost32(uint32_t netlong);

// ============================================================================
//  Socket Functions
// ============================================================================

/**
 * @brief Initialize socket subsystem (required on Windows, no-op on Linux).
 * @return 0 on success, -1 on failure.
 */
int socketInit();

/** @brief Cleanup socket subsystem. */
void socketCleanup();

/**
 * @brief Create a TCP socket.
 * @return Socket handle, or kInvalidSocket on failure.
 */
SocketHandle socketCreate();

/**
 * @brief Close a socket.
 * @param sock Socket handle.
 * @return 0 on success, -1 on failure.
 */
int socketClose(SocketHandle sock);

/**
 * @brief Set socket to non-blocking mode.
 * @param sock Socket handle.
 * @param enable true to enable non-blocking, false for blocking.
 * @return 0 on success, -1 on failure.
 */
int socketSetNonblocking(SocketHandle sock, bool enable);

/**
 * @brief Connect to a remote address.
 * @param sock Socket handle.
 * @param address IP address string (e.g., "192.168.1.1").
 * @param port Port number.
 * @return 0 on success, ITSCAM_SOCK_EINPROGRESS if non-blocking and in progress,
 *         or negative error code on failure.
 */
int socketConnect(SocketHandle sock, const char* address, uint16_t port);

/**
 * @brief Wait for socket to become ready (for connect completion or I/O).
 * @param sock Socket handle.
 * @param timeoutMs Timeout in milliseconds (-1 for infinite).
 * @param checkWrite true to check for write ready, false for read ready.
 * @return 1 if ready, 0 if timeout, -1 on error.
 */
int socketWait(SocketHandle sock, int timeoutMs, bool checkWrite);

/**
 * @brief Check if socket has an error (use after socketWait for connect).
 * @param sock Socket handle.
 * @return 0 if no error, or error code.
 */
int socketGetError(SocketHandle sock);

/**
 * @brief Read data from socket.
 * @param sock Socket handle.
 * @param buf Buffer to receive data.
 * @param len Maximum bytes to read.
 * @return Number of bytes read, 0 on connection closed, -1 on error.
 */
int64_t socketRead(SocketHandle sock, void* buf, size_t len);

/**
 * @brief Send data on socket.
 * @param sock Socket handle.
 * @param buf Data to send.
 * @param len Number of bytes to send.
 * @return Number of bytes sent, or -1 on error.
 */
int64_t socketSend(SocketHandle sock, const void* buf, size_t len);

/**
 * @brief Get the last socket error code (platform-independent).
 * @return One of ITSCAM_SOCK_* error codes.
 */
int socketErrno();

/**
 * @brief Get error message for a socket error code.
 * @param err Error code (from socketErrno).
 * @return Human-readable error string (thread-local buffer).
 */
const char* socketStrerror(int err);

// ============================================================================
//  Thread Functions
// ============================================================================

/**
 * @brief Create and start a new thread.
 * @param thread Pointer to receive thread handle.
 * @param func Thread function to execute.
 * @param arg Argument passed to thread function.
 * @return 0 on success, -1 on failure.
 */
int threadCreate(ThreadHandle* thread, ThreadFunc func, void* arg);

/**
 * @brief Wait for a thread to finish.
 * @param thread Thread handle.
 * @return 0 on success, -1 on failure.
 */
int threadJoin(ThreadHandle thread);

/**
 * @brief Detach a thread (it will clean up automatically when finished).
 * @param thread Thread handle.
 * @return 0 on success, -1 on failure.
 */
int threadDetach(ThreadHandle thread);

/**
 * @brief Check if a thread handle is valid/joinable.
 * @param thread Thread handle.
 * @return true if joinable, false if not.
 */
bool threadJoinable(ThreadHandle thread);

/**
 * @brief Sleep the current thread (low-level OS call).
 * @param ms Milliseconds to sleep.
 */
void sleepMs(uint32_t ms);

/**
 * @brief Sleep the current thread.
 * @param ms Milliseconds to sleep.
 *
 * Prefer this over std::this_thread::sleep_for in application code: it
 * works on every supported platform, including MinGW builds that use the
 * win32 (not posix) threading model.
 */
inline void sleepForMs(uint32_t ms) {
    sleepMs(ms);
}

/** @brief Yield the current thread's time slice. */
void threadYield();

// ============================================================================
//  Mutex Functions
// ============================================================================

/**
 * @brief Create a mutex.
 * @param mutex Pointer to receive mutex handle.
 * @return 0 on success, -1 on failure.
 */
int mutexCreate(MutexHandle* mutex);

/**
 * @brief Destroy a mutex.
 * @param mutex Mutex handle.
 */
void mutexDestroy(MutexHandle mutex);

/**
 * @brief Lock a mutex (blocking).
 * @param mutex Mutex handle.
 * @return 0 on success, -1 on failure.
 */
int mutexLock(MutexHandle mutex);

/**
 * @brief Unlock a mutex.
 * @param mutex Mutex handle.
 * @return 0 on success, -1 on failure.
 */
int mutexUnlock(MutexHandle mutex);

/**
 * @brief Try to lock a mutex without blocking.
 * @param mutex Mutex handle.
 * @return 0 if locked successfully, 1 if would block, -1 on error.
 */
int mutexTrylock(MutexHandle mutex);

// ============================================================================
//  Condition Variable Functions
// ============================================================================

/**
 * @brief Create a condition variable.
 * @param cond Pointer to receive condition variable handle.
 * @return 0 on success, -1 on failure.
 */
int condCreate(CondHandle* cond);

/**
 * @brief Destroy a condition variable.
 * @param cond Condition variable handle.
 */
void condDestroy(CondHandle cond);

/**
 * @brief Wait on a condition variable.
 * @param cond Condition variable handle.
 * @param mutex Mutex handle (must be locked by caller).
 * @return 0 on success, -1 on failure.
 */
int condWait(CondHandle cond, MutexHandle mutex);

/**
 * @brief Wait on a condition variable with timeout.
 * @param cond Condition variable handle.
 * @param mutex Mutex handle (must be locked by caller).
 * @param timeoutMs Timeout in milliseconds.
 * @return 0 if signaled, 1 if timeout, -1 on error.
 */
int condTimedwait(CondHandle cond, MutexHandle mutex, uint32_t timeoutMs);

/**
 * @brief Signal one waiting thread.
 * @param cond Condition variable handle.
 * @return 0 on success, -1 on failure.
 */
int condSignal(CondHandle cond);

/**
 * @brief Signal all waiting threads.
 * @param cond Condition variable handle.
 * @return 0 on success, -1 on failure.
 */
int condBroadcast(CondHandle cond);

// ============================================================================
//  Time Functions
// ============================================================================

/**
 * @brief Get current monotonic time in milliseconds.
 * @return Milliseconds since an arbitrary point (suitable for measuring intervals).
 */
uint64_t monotonicMs();

/**
 * @brief Get current time in milliseconds since Unix epoch.
 * @return Milliseconds since January 1, 1970 UTC.
 */
uint64_t epochMs();

// ============================================================================
//  C++ RAII Wrappers
// ============================================================================

/**
 * @brief RAII mutex wrapper (similar to std::mutex).
 */
class Mutex {
public:
    Mutex() : m_handle(nullptr) {
        mutexCreate(&m_handle);
    }
    ~Mutex() {
        if (m_handle) mutexDestroy(m_handle);
    }

    // Non-copyable
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    // Movable
    Mutex(Mutex&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }
    Mutex& operator=(Mutex&& other) noexcept {
        if (this != &other) {
            if (m_handle) mutexDestroy(m_handle);
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    void lock() { mutexLock(m_handle); }
    void unlock() { mutexUnlock(m_handle); }
    bool tryLock() { return mutexTrylock(m_handle) == 0; }

    MutexHandle nativeHandle() const { return m_handle; }

private:
    MutexHandle m_handle;
};

/**
 * @brief RAII lock guard (similar to std::lock_guard).
 */
template<typename MutexType>
class LockGuard {
public:
    explicit LockGuard(MutexType& mutex) : m_mutex(mutex) {
        m_mutex.lock();
    }
    ~LockGuard() {
        m_mutex.unlock();
    }

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    MutexType& m_mutex;
};

/**
 * @brief RAII unique lock (similar to std::unique_lock).
 */
template<typename MutexType>
class UniqueLock {
public:
    explicit UniqueLock(MutexType& mutex) : m_mutex(&mutex), m_owns(true) {
        m_mutex->lock();
    }
    ~UniqueLock() {
        if (m_owns) m_mutex->unlock();
    }

    UniqueLock(const UniqueLock&) = delete;
    UniqueLock& operator=(const UniqueLock&) = delete;

    UniqueLock(UniqueLock&& other) noexcept
        : m_mutex(other.m_mutex), m_owns(other.m_owns) {
        other.m_mutex = nullptr;
        other.m_owns = false;
    }

    void lock() {
        m_mutex->lock();
        m_owns = true;
    }

    void unlock() {
        m_mutex->unlock();
        m_owns = false;
    }

    bool ownsLock() const { return m_owns; }
    MutexType* mutex() const { return m_mutex; }

private:
    MutexType* m_mutex;
    bool m_owns;
};

/**
 * @brief Condition variable wrapper (similar to std::condition_variable).
 */
class ConditionVariable {
public:
    ConditionVariable() : m_handle(nullptr) {
        condCreate(&m_handle);
    }
    ~ConditionVariable() {
        if (m_handle) condDestroy(m_handle);
    }

    // Non-copyable
    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

    void notifyOne() { condSignal(m_handle); }
    void notifyAll() { condBroadcast(m_handle); }

    void wait(UniqueLock<Mutex>& lock) {
        condWait(m_handle, lock.mutex()->nativeHandle());
    }

    template<typename Predicate>
    void wait(UniqueLock<Mutex>& lock, Predicate pred) {
        while (!pred()) {
            wait(lock);
        }
    }

    /**
     * @brief Wait with timeout.
     * @return CvStatus::timeout if timed out, CvStatus::noTimeout if signaled.
     */
    enum class CvStatus { noTimeout, timeout };

    CvStatus waitFor(UniqueLock<Mutex>& lock, uint32_t timeoutMs) {
        int result = condTimedwait(m_handle, lock.mutex()->nativeHandle(), timeoutMs);
        return (result == 1) ? CvStatus::timeout : CvStatus::noTimeout;
    }

    template<typename Predicate>
    bool waitFor(UniqueLock<Mutex>& lock, uint32_t timeoutMs, Predicate pred) {
        while (!pred()) {
            if (waitFor(lock, timeoutMs) == CvStatus::timeout) {
                return pred();  // Check one more time after timeout
            }
        }
        return true;
    }

    CondHandle nativeHandle() const { return m_handle; }

private:
    CondHandle m_handle;
};

/**
 * @brief Thread wrapper (similar to std::thread).
 */
class Thread {
public:
    Thread() : m_handle(nullptr) {}

    template<typename Func, typename... Args>
    explicit Thread(Func&& func, Args&&... args) : m_handle(nullptr) {
        // Create a callable wrapper that captures the function and arguments
        auto* wrapper = new ThreadWrapper<Func, Args...>(
            std::forward<Func>(func), std::forward<Args>(args)...);
        
        if (threadCreate(&m_handle, &Thread::threadEntry<Func, Args...>, wrapper) != 0) {
            delete wrapper;
            m_handle = nullptr;
        }
    }

    ~Thread() {
        // If thread is joinable and we're destructing, the behavior should match std::thread
        // which calls std::terminate(). We'll just detach to avoid crashes.
        if (joinable()) {
            threadDetach(m_handle);
        }
    }

    // Non-copyable
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    // Movable
    Thread(Thread&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    Thread& operator=(Thread&& other) noexcept {
        if (this != &other) {
            if (joinable()) {
                threadDetach(m_handle);
            }
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    bool joinable() const {
        return m_handle != nullptr && threadJoinable(m_handle);
    }

    void join() {
        if (m_handle) {
            threadJoin(m_handle);
            m_handle = nullptr;
        }
    }

    void detach() {
        if (m_handle) {
            threadDetach(m_handle);
            m_handle = nullptr;
        }
    }

    ThreadHandle nativeHandle() const { return m_handle; }

private:
    ThreadHandle m_handle;

    // Thread wrapper to capture callable and arguments
    template<typename Func, typename... Args>
    struct ThreadWrapper {
        Func func;
        std::tuple<Args...> args;

        template<typename F, typename... A>
        ThreadWrapper(F&& f, A&&... a)
            : func(std::forward<F>(f))
            , args(std::forward<A>(a)...) {}

        void invoke() {
            invokeImpl(std::index_sequence_for<Args...>{});
        }

    private:
        template<size_t... I>
        void invokeImpl(std::index_sequence<I...>) {
            func(std::get<I>(args)...);
        }
    };

    template<typename Func, typename... Args>
    static void threadEntry(void* arg) {
        auto* wrapper = static_cast<ThreadWrapper<Func, Args...>*>(arg);
        wrapper->invoke();
        delete wrapper;
    }
};

/**
 * @brief Yield current thread.
 */
inline void yield() {
    threadYield();
}

/**
 * @brief Get monotonic time in milliseconds.
 */
inline uint64_t monotonicNow() {
    return monotonicMs();
}

}  // namespace itscam_os

#endif  // ITSCAM_OS_H
