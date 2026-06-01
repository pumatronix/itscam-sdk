/*
 *  itscam_os_windows.cpp
 *
 *  ITSCAM Client SDK - Windows Implementation
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Implementation using Win32 API:
 *  - Threads: CreateThread, WaitForSingleObject
 *  - Mutexes: CRITICAL_SECTION (faster than Mutex for same-process)
 *  - Condition Variables: CONDITION_VARIABLE (Vista+)
 *  - Sockets: Winsock2
 */

#if defined(_WIN32) || defined(__MINGW32__)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Require Windows Vista or later for CONDITION_VARIABLE
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include "itscam_os.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <cstring>

// Link with Winsock library (for MSVC; MinGW uses -lws2_32)
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

namespace itscam {
namespace os {

//=========================================================================
// Internal Structures
//=========================================================================

struct ThreadImpl {
    HANDLE handle;
    ThreadFunc func;
    void* arg;
    std::atomic<int> refCount;
    std::atomic<int> state;
    std::atomic<int> handleClosed;
};

enum ThreadState {
    THREAD_JOINABLE = 0,
    THREAD_DETACHED = 1,
    THREAD_JOINED = 2,
};

struct MutexImpl {
    CRITICAL_SECTION cs;
};

struct CondImpl {
    CONDITION_VARIABLE cv;
};

// Thread-local error buffer
static thread_local char s_errorBuffer[256] = {0};

// Winsock initialization tracking
static volatile LONG s_winsockInitialized = 0;

static void threadRelease(ThreadImpl* impl) {
    if (impl && impl->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        free(impl);
    }
}

static void closeThreadHandle(ThreadImpl* impl) {
    if (impl && impl->handleClosed.exchange(1, std::memory_order_acq_rel) == 0) {
        CloseHandle(impl->handle);
    }
}

//=========================================================================
// Thread Entry Point
//=========================================================================

static DWORD WINAPI threadEntryPoint(LPVOID arg) {
    ThreadImpl* impl = static_cast<ThreadImpl*>(arg);
    impl->func(impl->arg);

    threadRelease(impl);

    return 0;
}

//=========================================================================
// Socket Functions
//=========================================================================

int socketInit() {
    // Use interlocked compare/exchange for thread-safe initialization
    if (InterlockedCompareExchange(&s_winsockInitialized, 1, 0) == 0) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            s_winsockInitialized = 0;
            return -1;
        }
    }
    return 0;
}

void socketCleanup() {
    if (InterlockedCompareExchange(&s_winsockInitialized, 0, 1) == 1) {
        WSACleanup();
    }
}

//=========================================================================
// Byte Order Functions
//=========================================================================

uint16_t hostToNet16(uint16_t hostshort) {
    return ::htons(hostshort);
}

uint32_t hostToNet32(uint32_t hostlong) {
    return ::htonl(hostlong);
}

uint16_t netToHost16(uint16_t netshort) {
    return ::ntohs(netshort);
}

uint32_t netToHost32(uint32_t netlong) {
    return ::ntohl(netlong);
}

//=========================================================================
// Socket Functions
//=========================================================================

SocketHandle socketCreate() {
    // Ensure Winsock is initialized
    if (!s_winsockInitialized) {
        if (socketInit() != 0) {
            return ITSCAM_OS_INVALID_SOCKET;
        }
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return ITSCAM_OS_INVALID_SOCKET;
    }

    // Disable Nagle's algorithm for lower latency
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));

    return (SocketHandle)sock;
}

int socketClose(SocketHandle sock) {
    return closesocket((SOCKET)sock);
}

int socketSetNonblocking(SocketHandle sock, bool enable) {
    u_long mode = enable ? 1 : 0;
    return ioctlsocket((SOCKET)sock, FIONBIO, &mode);
}

// inet_pton implementation for older MinGW versions
static int itscam_inet_pton(int af, const char* src, void* dst) {
    if (af == AF_INET) {
        struct sockaddr_in sa;
        int size = sizeof(sa);
        char addr_copy[64];
        strncpy(addr_copy, src, sizeof(addr_copy) - 1);
        addr_copy[sizeof(addr_copy) - 1] = '\0';

        if (WSAStringToAddressA(addr_copy, AF_INET, NULL, (LPSOCKADDR)&sa, &size) == 0) {
            memcpy(dst, &sa.sin_addr, sizeof(sa.sin_addr));
            return 1;
        }
        return 0;
    } else if (af == AF_INET6) {
        struct sockaddr_in6 sa;
        int size = sizeof(sa);
        char addr_copy[64];
        strncpy(addr_copy, src, sizeof(addr_copy) - 1);
        addr_copy[sizeof(addr_copy) - 1] = '\0';

        if (WSAStringToAddressA(addr_copy, AF_INET6, NULL, (LPSOCKADDR)&sa, &size) == 0) {
            memcpy(dst, &sa.sin6_addr, sizeof(sa.sin6_addr));
            return 1;
        }
        return 0;
    }
    return -1;
}

int socketConnect(SocketHandle sock, const char* address, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = ::htons(port);

    if (itscam_inet_pton(AF_INET, address, &addr.sin_addr) <= 0) {
        return -1;
    }

    int result = connect((SOCKET)sock, (struct sockaddr*)&addr, sizeof(addr));
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS) {
            return ITSCAM_SOCK_EINPROGRESS;
        }
        return -1;
    }

    return 0;
}

int socketWait(SocketHandle sock, int timeoutMs, bool checkWrite) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET((SOCKET)sock, &fds);

    struct timeval tv;
    struct timeval* ptv = NULL;

    if (timeoutMs >= 0) {
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        ptv = &tv;
    }

    int result;
    if (checkWrite) {
        result = select(0, NULL, &fds, NULL, ptv);  // First arg ignored on Windows
    } else {
        result = select(0, &fds, NULL, NULL, ptv);
    }

    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEINTR) return 0;  // Interrupted, treat as timeout
        return -1;
    }

    return (result > 0) ? 1 : 0;
}

int socketGetError(SocketHandle sock) {
    int error = 0;
    int len = sizeof(error);
    if (getsockopt((SOCKET)sock, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    return error;
}

int64_t socketRead(SocketHandle sock, void* buf, size_t len) {
    int result = recv((SOCKET)sock, (char*)buf, (int)len, 0);
    return (result == SOCKET_ERROR) ? -1 : result;
}

int64_t socketSend(SocketHandle sock, const void* buf, size_t len) {
    int result = send((SOCKET)sock, (const char*)buf, (int)len, 0);
    return (result == SOCKET_ERROR) ? -1 : result;
}

int socketErrno() {
    int e = WSAGetLastError();
    switch (e) {
        case 0:
            return ITSCAM_SOCK_OK;
        case WSAEWOULDBLOCK:
            return ITSCAM_SOCK_EWOULDBLOCK;
        case WSAEINPROGRESS:
        case WSAEALREADY:
            return ITSCAM_SOCK_EINPROGRESS;
        case WSAECONNREFUSED:
            return ITSCAM_SOCK_ECONNREFUSED;
        case WSAETIMEDOUT:
            return ITSCAM_SOCK_ETIMEDOUT;
        case WSAENETUNREACH:
        case WSAEHOSTUNREACH:
            return ITSCAM_SOCK_ENETUNREACH;
        case WSAECONNRESET:
            return ITSCAM_SOCK_ECONNRESET;
        case WSAEINTR:
            return ITSCAM_SOCK_EINTR;
        default:
            return ITSCAM_SOCK_EUNKNOWN;
    }
}

const char* socketStrerror(int err) {
    switch (err) {
        case ITSCAM_SOCK_OK:
            return "Success";
        case ITSCAM_SOCK_EWOULDBLOCK:
            return "Operation would block";
        case ITSCAM_SOCK_EINPROGRESS:
            return "Operation in progress";
        case ITSCAM_SOCK_ECONNREFUSED:
            return "Connection refused";
        case ITSCAM_SOCK_ETIMEDOUT:
            return "Connection timed out";
        case ITSCAM_SOCK_ENETUNREACH:
            return "Network unreachable";
        case ITSCAM_SOCK_ECONNRESET:
            return "Connection reset by peer";
        case ITSCAM_SOCK_EPIPE:
            return "Connection closed";
        case ITSCAM_SOCK_EINTR:
            return "Interrupted system call";
        default:
            snprintf(s_errorBuffer, sizeof(s_errorBuffer), "Unknown error %d", err);
            return s_errorBuffer;
    }
}

//=========================================================================
// Thread Functions
//=========================================================================

int threadCreate(ThreadHandle* thread, ThreadFunc func, void* arg) {
    ThreadImpl* impl = static_cast<ThreadImpl*>(malloc(sizeof(ThreadImpl)));
    if (!impl) return -1;

    impl->func = func;
    impl->arg = arg;
    impl->refCount.store(2, std::memory_order_relaxed);
    impl->state.store(THREAD_JOINABLE, std::memory_order_relaxed);
    impl->handleClosed.store(0, std::memory_order_relaxed);

    impl->handle = CreateThread(
        NULL,              // Default security attributes
        0,                 // Default stack size
        threadEntryPoint,  // Thread function
        impl,              // Argument to thread function
        0,                 // Run immediately
        NULL               // Don't return thread ID
    );

    if (impl->handle == NULL) {
        free(impl);
        return -1;
    }

    *thread = impl;
    return 0;
}

int threadJoin(ThreadHandle thread) {
    if (!thread) {
        return -1;
    }

    int expected = THREAD_JOINABLE;
    if (!thread->state.compare_exchange_strong(expected, THREAD_JOINED,
                                               std::memory_order_acq_rel)) {
        return -1;
    }

    DWORD result = WaitForSingleObject(thread->handle, INFINITE);
    if (result != WAIT_OBJECT_0) {
        thread->state.store(THREAD_JOINABLE, std::memory_order_release);
        return -1;
    }

    closeThreadHandle(thread);
    threadRelease(thread);
    return 0;
}

int threadDetach(ThreadHandle thread) {
    if (!thread) {
        return -1;
    }

    int expected = THREAD_JOINABLE;
    if (!thread->state.compare_exchange_strong(expected, THREAD_DETACHED,
                                               std::memory_order_acq_rel)) {
        if (expected == THREAD_DETACHED) {
            return 0;
        }
        return -1;
    }

    closeThreadHandle(thread);
    threadRelease(thread);
    return 0;
}

bool threadJoinable(ThreadHandle thread) {
    return thread && thread->state.load(std::memory_order_acquire) == THREAD_JOINABLE;
}

void sleepMs(uint32_t ms) {
    Sleep(ms);
}

void threadYield() {
    SwitchToThread();
}

//=========================================================================
// Mutex Functions
//=========================================================================

int mutexCreate(MutexHandle* mutex) {
    MutexImpl* impl = static_cast<MutexImpl*>(malloc(sizeof(MutexImpl)));
    if (!impl) return -1;

    // CRITICAL_SECTION is recursive by default
    InitializeCriticalSection(&impl->cs);

    *mutex = impl;
    return 0;
}

void mutexDestroy(MutexHandle mutex) {
    if (mutex) {
        DeleteCriticalSection(&mutex->cs);
        free(mutex);
    }
}

int mutexLock(MutexHandle mutex) {
    EnterCriticalSection(&mutex->cs);
    return 0;
}

int mutexUnlock(MutexHandle mutex) {
    LeaveCriticalSection(&mutex->cs);
    return 0;
}

int mutexTrylock(MutexHandle mutex) {
    if (TryEnterCriticalSection(&mutex->cs)) {
        return 0;   // Locked
    }
    return 1;       // Would block
}

//=========================================================================
// Condition Variable Functions
//=========================================================================

int condCreate(CondHandle* cond) {
    CondImpl* impl = static_cast<CondImpl*>(malloc(sizeof(CondImpl)));
    if (!impl) return -1;

    InitializeConditionVariable(&impl->cv);

    *cond = impl;
    return 0;
}

void condDestroy(CondHandle cond) {
    if (cond) {
        // CONDITION_VARIABLE doesn't need explicit destruction
        free(cond);
    }
}

int condWait(CondHandle cond, MutexHandle mutex) {
    if (SleepConditionVariableCS(&cond->cv, &mutex->cs, INFINITE)) {
        return 0;
    }
    return -1;
}

int condTimedwait(CondHandle cond, MutexHandle mutex, uint32_t timeoutMs) {
    if (SleepConditionVariableCS(&cond->cv, &mutex->cs, timeoutMs)) {
        return 0;   // Signaled
    }
    
    DWORD err = GetLastError();
    if (err == ERROR_TIMEOUT) {
        return 1;   // Timeout
    }
    return -1;      // Error
}

int condSignal(CondHandle cond) {
    WakeConditionVariable(&cond->cv);
    return 0;
}

int condBroadcast(CondHandle cond) {
    WakeAllConditionVariable(&cond->cv);
    return 0;
}

//=========================================================================
// Time Functions
//=========================================================================

uint64_t monotonicMs() {
    // Use QueryPerformanceCounter for high precision
    static LARGE_INTEGER frequency = {};
    static BOOL hasFrequency = FALSE;

    if (!hasFrequency) {
        QueryPerformanceFrequency(&frequency);
        hasFrequency = TRUE;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    // Convert to milliseconds
    return (uint64_t)(counter.QuadPart * 1000 / frequency.QuadPart);
}

uint64_t epochMs() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    // FILETIME is 100-nanosecond intervals since Jan 1, 1601
    // Unix epoch is Jan 1, 1970
    // Difference is 11644473600 seconds
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    // Convert to milliseconds since Unix epoch
    return (uint64_t)((uli.QuadPart / 10000) - 11644473600000ULL);
}

}  // namespace os
}  // namespace itscam

//=========================================================================
// Auto-initialize Winsock on DLL load
//=========================================================================

#ifdef ITSCAM_SDK_BUILDING

// Initialize Winsock when the DLL loads
static class WinsockAutoInit {
public:
    WinsockAutoInit() {
        itscam::os::socketInit();
    }
    ~WinsockAutoInit() {
        itscam::os::socketCleanup();
    }
} s_winsockAutoInit;

#endif  // ITSCAM_SDK_BUILDING

#endif  // _WIN32 || __MINGW32__
