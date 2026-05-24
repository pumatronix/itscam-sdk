/*
 *  itscam_mingw_stl_sync.h
 *
 *  Minimal std::thread / mutex / condition_variable shims for MinGW builds
 *  that use the win32 (not posix) threading model.  libstdc++ omits <thread>
 *  in that configuration; cpp-httplib still needs the types.
 *
 *  Only included when ITSCAM_MINGW_WIN32_THREADS is defined by the build.
 *
 *  Copyright (c) 2026 Pumatronix
 */
#ifndef ITSCAM_MINGW_STL_SYNC_H
#define ITSCAM_MINGW_STL_SYNC_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <windows.h>

namespace std {

class mutex {
public:
    mutex() { InitializeCriticalSection(&cs_); }
    ~mutex() { DeleteCriticalSection(&cs_); }
    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

    void lock() { EnterCriticalSection(&cs_); }
    void unlock() { LeaveCriticalSection(&cs_); }
    bool try_lock() { return TryEnterCriticalSection(&cs_) != 0; }

    CRITICAL_SECTION* native_handle() { return &cs_; }

private:
    CRITICAL_SECTION cs_;
};

using recursive_mutex = mutex;

class condition_variable {
public:
    condition_variable() { InitializeConditionVariable(&cv_); }

    void notify_one() { WakeConditionVariable(&cv_); }
    void notify_all() { WakeAllConditionVariable(&cv_); }

    template<typename Lock>
    void wait(Lock& lock) {
        SleepConditionVariableCS(&cv_, lock.mutex()->native_handle(), INFINITE);
    }

    template<typename Lock, typename Pred>
    void wait(Lock& lock, Pred pred) {
        while (!pred()) { wait(lock); }
    }

private:
    CONDITION_VARIABLE cv_;
};

struct thread {
    struct id {
        DWORD value = 0;
        id() = default;
        explicit id(DWORD v) : value(v) {}
        bool operator==(const id& o) const { return value == o.value; }
        bool operator!=(const id& o) const { return !(*this == o); }
    };

    thread() = default;

    template<typename F>
    explicit thread(F&& f) {
        using Fn = std::decay_t<F>;
        auto ctx = new Context<Fn>{Fn(std::forward<F>(f))};
        handle_ = CreateThread(
            nullptr, 0, &thread::trampoline<Fn>, ctx, 0, &id_.value);
    }

    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;

    thread(thread&& o) noexcept
        : handle_(o.handle_), id_(o.id_) {
        o.handle_ = nullptr;
        o.id_ = id{};
    }

    thread& operator=(thread&& o) noexcept {
        if (this != &o) {
            if (joinable()) { join(); }
            handle_ = o.handle_;
            id_ = o.id_;
            o.handle_ = nullptr;
            o.id_ = id{};
        }
        return *this;
    }

    ~thread() {
        if (joinable()) { join(); }
    }

    bool joinable() const { return handle_ != nullptr; }

    void join() {
        if (handle_) {
            WaitForSingleObject(handle_, INFINITE);
            CloseHandle(handle_);
            handle_ = nullptr;
        }
    }

    id get_id() const { return id_; }

    static unsigned hardware_concurrency() {
        SYSTEM_INFO info{};
        GetNativeSystemInfo(&info);
        return info.dwNumberOfProcessors ? info.dwNumberOfProcessors : 1;
    }

private:
    template<typename Fn>
    struct Context {
        Fn fn;
    };

    template<typename Fn>
    static DWORD WINAPI trampoline(LPVOID arg) {
        auto ctx = static_cast<Context<Fn>*>(arg);
        ctx->fn();
        delete ctx;
        return 0;
    }

    HANDLE handle_ = nullptr;
    id id_{};
};

namespace this_thread {

inline thread::id get_id() {
    return thread::id(GetCurrentThreadId());
}

template<typename Rep, typename Period>
inline void sleep_for(const std::chrono::duration<Rep, Period>& d) {
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    if (ms > 0) {
        Sleep(static_cast<DWORD>(ms));
    } else {
        Sleep(1);
    }
}

} // namespace this_thread

struct once_flag {
    mutex mtx;
    long done = 0;
};

template<typename Callable>
inline void call_once(once_flag& flag, Callable&& fn) {
    if (flag.done) return;
    flag.mtx.lock();
    if (!flag.done) {
        fn();
        flag.done = 1;
    }
    flag.mtx.unlock();
}

} // namespace std

#endif // ITSCAM_MINGW_STL_SYNC_H
