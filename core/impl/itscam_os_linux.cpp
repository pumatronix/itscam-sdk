/*
 *  itscam_os_linux.cpp
 *
 *  ITSCAM Client SDK - Linux/POSIX Implementation
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Implementation using POSIX threads (pthread) and BSD sockets.
 */

#if !defined(_WIN32) && !defined(__MINGW32__)

#include "../itscam_os.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>

namespace itscam_os {

// ============================================================================
//  Internal Structures
// ============================================================================

struct ThreadImpl {
    pthread_t thread;
    ThreadFunc func;
    void* arg;
    std::atomic<int> refCount;
    std::atomic<int> state;
};

enum ThreadState {
    THREAD_JOINABLE = 0,
    THREAD_DETACHED = 1,
    THREAD_JOINED = 2,
};

struct MutexImpl {
    pthread_mutex_t mutex;
};

struct CondImpl {
    pthread_cond_t cond;
};

// Thread-local error buffer
static thread_local char s_errorBuffer[256] = {0};

static void threadRelease(ThreadImpl* impl) {
    if (impl && impl->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        free(impl);
    }
}

// ============================================================================
//  Thread Entry Point
// ============================================================================

static void* threadEntryPoint(void* arg) {
    ThreadImpl* impl = static_cast<ThreadImpl*>(arg);
    impl->func(impl->arg);

    threadRelease(impl);
    return nullptr;
}

// ============================================================================
//  Socket Functions
// ============================================================================

int socketInit() {
    // No initialization needed on Linux
    return 0;
}

void socketCleanup() {
    // No cleanup needed on Linux
}

// ============================================================================
//  Byte Order Functions
// ============================================================================

uint16_t hostToNet16(uint16_t hostshort) {
    return htons(hostshort);
}

uint32_t hostToNet32(uint32_t hostlong) {
    return htonl(hostlong);
}

uint16_t netToHost16(uint16_t netshort) {
    return ntohs(netshort);
}

uint32_t netToHost32(uint32_t netlong) {
    return ntohl(netlong);
}

// ============================================================================
//  Socket Functions
// ============================================================================

SocketHandle socketCreate() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return ITSCAM_OS_INVALID_SOCKET;
    }

    // Disable Nagle's algorithm for lower latency
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    return sock;
}

int socketClose(SocketHandle sock) {
    return close(sock);
}

int socketSetNonblocking(SocketHandle sock, bool enable) {
    int flags = fcntl(sock, F_GETFL);
    if (flags < 0) return -1;
    
    if (enable) {
        return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    } else {
        return fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
    }
}

int socketConnect(SocketHandle sock, const char* address, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, address, &addr.sin_addr) <= 0) {
        return -1;
    }

    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (result < 0) {
        if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
            return ITSCAM_SOCK_EINPROGRESS;
        }
        return -1;
    }

    return 0;
}

int socketWait(SocketHandle sock, int timeoutMs, bool checkWrite) {
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = static_cast<short>(checkWrite ? POLLOUT : POLLIN);
    pfd.revents = 0;

    int result;
    do {
        result = poll(&pfd, 1, timeoutMs);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        return -1;
    }

    if (result == 0) {
        return 0;
    }

    return (pfd.revents != 0) ? 1 : 0;
}

int socketGetError(SocketHandle sock) {
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        return errno;
    }
    return error;
}

int64_t socketRead(SocketHandle sock, void* buf, size_t len) {
    return read(sock, buf, len);
}

int64_t socketSend(SocketHandle sock, const void* buf, size_t len) {
    return send(sock, buf, len, MSG_NOSIGNAL);
}

int socketErrno() {
    int e = errno;
    switch (e) {
        case EWOULDBLOCK:
#if EAGAIN != EWOULDBLOCK
        case EAGAIN:
#endif
            return ITSCAM_SOCK_EWOULDBLOCK;
        case EINPROGRESS:
            return ITSCAM_SOCK_EINPROGRESS;
        case ECONNREFUSED:
            return ITSCAM_SOCK_ECONNREFUSED;
        case ETIMEDOUT:
            return ITSCAM_SOCK_ETIMEDOUT;
        case ENETUNREACH:
        case EHOSTUNREACH:
            return ITSCAM_SOCK_ENETUNREACH;
        case ECONNRESET:
            return ITSCAM_SOCK_ECONNRESET;
        case EPIPE:
            return ITSCAM_SOCK_EPIPE;
        case EINTR:
            return ITSCAM_SOCK_EINTR;
        case 0:
            return ITSCAM_SOCK_OK;
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
            return "Broken pipe";
        case ITSCAM_SOCK_EINTR:
            return "Interrupted system call";
        default:
            snprintf(s_errorBuffer, sizeof(s_errorBuffer), "Unknown error %d", err);
            return s_errorBuffer;
    }
}

// ============================================================================
//  Thread Functions
// ============================================================================

int threadCreate(ThreadHandle* thread, ThreadFunc func, void* arg) {
    ThreadImpl* impl = static_cast<ThreadImpl*>(malloc(sizeof(ThreadImpl)));
    if (!impl) return -1;

    impl->func = func;
    impl->arg = arg;
    impl->refCount.store(2, std::memory_order_relaxed);
    impl->state.store(THREAD_JOINABLE, std::memory_order_relaxed);

    int result = pthread_create(&impl->thread, nullptr, threadEntryPoint, impl);
    if (result != 0) {
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

    int result = pthread_join(thread->thread, nullptr);
    if (result != 0) {
        thread->state.store(THREAD_JOINABLE, std::memory_order_release);
        return -1;
    }

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

    int result = pthread_detach(thread->thread);
    if (result != 0) {
        thread->state.store(THREAD_JOINABLE, std::memory_order_release);
        return -1;
    }

    threadRelease(thread);
    return 0;
}

bool threadJoinable(ThreadHandle thread) {
    return thread && thread->state.load(std::memory_order_acquire) == THREAD_JOINABLE;
}

void sleepMs(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
        // Continue sleeping if interrupted
    }
}

void threadYield() {
    sched_yield();
}

// ============================================================================
//  Mutex Functions
// ============================================================================

int mutexCreate(MutexHandle* mutex) {
    MutexImpl* impl = static_cast<MutexImpl*>(malloc(sizeof(MutexImpl)));
    if (!impl) return -1;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    int result = pthread_mutex_init(&impl->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    if (result != 0) {
        free(impl);
        return -1;
    }

    *mutex = impl;
    return 0;
}

void mutexDestroy(MutexHandle mutex) {
    if (mutex) {
        pthread_mutex_destroy(&mutex->mutex);
        free(mutex);
    }
}

int mutexLock(MutexHandle mutex) {
    return (pthread_mutex_lock(&mutex->mutex) == 0) ? 0 : -1;
}

int mutexUnlock(MutexHandle mutex) {
    return (pthread_mutex_unlock(&mutex->mutex) == 0) ? 0 : -1;
}

int mutexTrylock(MutexHandle mutex) {
    int result = pthread_mutex_trylock(&mutex->mutex);
    if (result == 0) return 0;           // Locked
    if (result == EBUSY) return 1;       // Would block
    return -1;                           // Error
}

// ============================================================================
//  Condition Variable Functions
// ============================================================================

int condCreate(CondHandle* cond) {
    CondImpl* impl = static_cast<CondImpl*>(malloc(sizeof(CondImpl)));
    if (!impl) return -1;

    int result = pthread_cond_init(&impl->cond, nullptr);
    if (result != 0) {
        free(impl);
        return -1;
    }

    *cond = impl;
    return 0;
}

void condDestroy(CondHandle cond) {
    if (cond) {
        pthread_cond_destroy(&cond->cond);
        free(cond);
    }
}

int condWait(CondHandle cond, MutexHandle mutex) {
    return (pthread_cond_wait(&cond->cond, &mutex->mutex) == 0) ? 0 : -1;
}

int condTimedwait(CondHandle cond, MutexHandle mutex, uint32_t timeoutMs) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    ts.tv_sec += timeoutMs / 1000;
    ts.tv_nsec += (timeoutMs % 1000) * 1000000L;
    
    // Handle nanosecond overflow
    while (ts.tv_nsec >= 1000000000L) {
        ts.tv_nsec -= 1000000000L;
        ts.tv_sec++;
    }

    int result = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &ts);
    if (result == 0) return 0;           // Signaled
    if (result == ETIMEDOUT) return 1;   // Timeout
    return -1;                           // Error
}

int condSignal(CondHandle cond) {
    return (pthread_cond_signal(&cond->cond) == 0) ? 0 : -1;
}

int condBroadcast(CondHandle cond) {
    return (pthread_cond_broadcast(&cond->cond) == 0) ? 0 : -1;
}

// ============================================================================
//  Time Functions
// ============================================================================

uint64_t monotonicMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

uint64_t epochMs() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

}  // namespace itscam_os

#endif  // !_WIN32 && !__MINGW32__
