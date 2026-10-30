/*
 *  itscam_callback_queue.cpp
 *
 *  ITSCAM Client SDK - Callback delivery (internal)
 *
 *  Copyright (c) 2026 Pumatronix
 */

#include "itscam_callback_queue.h"
#include "../itscam_sdk_utils.h"

#include <algorithm>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace itscam {
namespace detail {

using Mutex = os::Mutex;
using ConditionVariable = os::ConditionVariable;
template<typename M> using LockGuard = os::LockGuard<M>;
template<typename M> using UniqueLock = os::UniqueLock<M>;

namespace {

unsigned defaultThreadCount() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    long cores = static_cast<long>(si.dwNumberOfProcessors);
#else
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    if (cores < 1) cores = 1;
    return static_cast<unsigned>(std::min<long>(cores, 4));
}

// An idle pool thread exits after this long, so an idle process keeps none
constexpr uint32_t kIdleExitMs = 10000;

// Threads shared by every client, started on demand.  A queue waits in
// mReady until a thread takes it; it runs one callback, then goes to the
// back of mReady if it has more, so a busy client cannot starve the others.
class CallbackPool {
public:
    static CallbackPool& instance() {
        // Never destroyed: its threads are detached and may outlive main()
        static CallbackPool* pool = new CallbackPool();
        return *pool;
    }

    // Returns false if no thread can run the queue: none exists and none
    // could start.  The queue is then not kept; the caller retries later.
    bool schedule(std::shared_ptr<CallbackQueue> queue) {
        LockGuard<Mutex> lk(mMtx);
        mReady.push_back(std::move(queue));
        if (mIdle > 0) mCv.notifyOne();
        // A woken thread takes one queue: start another while the ready
        // queues outnumber the idle threads
        if (mReady.size() > mIdle && mThreads < mMax) startThread();
        if (mThreads == 0) {
            mReady.pop_back();
            return false;
        }
        return true;
    }

    void setThreadCount(unsigned count) {
        LockGuard<Mutex> lk(mMtx);
        mMax = count ? count : defaultThreadCount();
        // Threads above the limit exit before taking another queue; start
        // the ones now allowed, for the queues the idle threads cannot take
        mCv.notifyAll();
        for (size_t started = 0;
             mThreads < mMax && mReady.size() > mIdle + started; ++started) {
            if (!startThread()) break;
        }
    }

private:
    CallbackPool() : mMax(defaultThreadCount()) {}

    // Called with mMtx held.  If it fails, a later schedule() retries.
    bool startThread() {
        ++mThreads;
        os::Thread t([this]() { run(); });
        if (!t.joinable()) {
            --mThreads;
            return false;
        }
        t.detach();
        return true;
    }

    void run() {
        UniqueLock<Mutex> lk(mMtx);
        for (;;) {
            // Above a lowered limit, exit before taking another queue
            if (mThreads > mMax) {
                --mThreads;
                return;
            }
            while (mReady.empty()) {
                if (mThreads > mMax) {
                    --mThreads;
                    return;
                }
                ++mIdle;
                auto st = mCv.waitFor(lk, kIdleExitMs);
                --mIdle;
                if (st == ConditionVariable::CvStatus::timeout && mReady.empty()) {
                    --mThreads;
                    return;
                }
            }
            if (mThreads > mMax) continue;  // exits at the top
            auto queue = std::move(mReady.front());
            mReady.pop_front();
            lk.unlock();
            queue->runNext();
            queue.reset();  // may delete the queue: without mMtx
            lk.lock();
        }
    }

    Mutex             mMtx;
    ConditionVariable mCv;
    std::deque<std::shared_ptr<CallbackQueue>> mReady;
    unsigned          mMax;
    unsigned          mThreads = 0;
    unsigned          mIdle = 0;
};

}  // namespace

thread_local const CallbackQueue* CallbackQueue::tCurrent = nullptr;

std::shared_ptr<CallbackQueue> CallbackQueue::create() {
    return std::shared_ptr<CallbackQueue>(new CallbackQueue());
}

bool CallbackQueue::post(std::function<void()> fn, size_t bytes, Kind kind) {
    // Dropped callbacks are destroyed without mMtx: they may release the
    // last reference to their client
    std::vector<Item> dropped;
    bool queued = false;
    bool schedule = false;
    {
        LockGuard<Mutex> lk(mMtx);
        if (mClosed) return false;
        if (kind == Kind::Frame) {
            // Any limit reached: make room by dropping the oldest frames
            while (mFrames > 0 &&
                   (mFrames >= kMaxFrames || mItems.size() >= kMaxPending ||
                    mBytes + bytes > kMaxPendingBytes)) {
                auto it = std::find_if(mItems.begin(), mItems.end(),
                                       [](const Item& i) { return i.kind == Kind::Frame; });
                mBytes -= it->bytes;
                --mFrames;
                ++mDropped;
                dropped.push_back(std::move(*it));
                mItems.erase(it);
            }
        }
        if (kind != Kind::Essential &&
            (mItems.size() >= kMaxPending || mBytes + bytes > kMaxPendingBytes)) {
            ++mDropped;
        } else {
            mItems.push_back(Item{std::move(fn), bytes, kind});
            mBytes += bytes;
            if (kind == Kind::Frame) ++mFrames;
            queued = true;
        }
        // Even when this one is dropped: a failed schedule may have left
        // queued callbacks waiting for a thread
        if (!mScheduled && !mItems.empty()) schedule = mScheduled = true;
    }
    if (schedule && !CallbackPool::instance().schedule(shared_from_this())) {
        // No thread: the next post retries
        LockGuard<Mutex> lk(mMtx);
        mScheduled = false;
    }
    return queued;
}

void CallbackQueue::setDropReporter(std::function<void(uint64_t)> reporter) {
    LockGuard<Mutex> lk(mMtx);
    mDropReporter = std::move(reporter);
}

void CallbackQueue::runNext() {
    Item item;
    {
        LockGuard<Mutex> lk(mMtx);
        if (mClosed || mItems.empty()) {
            mScheduled = false;
            return;
        }
        item = std::move(mItems.front());
        mItems.pop_front();
        mBytes -= item.bytes;
        if (item.kind == Kind::Frame) --mFrames;
        // Caught up after dropping some: report it next
        if (mItems.empty() && mDropped > 0 && mDropReporter) {
            auto report = [reporter = mDropReporter, n = mDropped]() { reporter(n); };
            mDropped = 0;
            mItems.push_back(Item{std::move(report), 0, Kind::Essential});
        }
        mRunning = true;
        ++mRunSeq;
    }

    const CallbackQueue* outer = tCurrent;
    tCurrent = this;
    try {
        item.fn();
    } catch (...) {
        // A callback must not take the pool thread down
    }
    item.fn = nullptr;  // releases what it holds before waiters resume
    tCurrent = outer;

    bool more;
    {
        LockGuard<Mutex> lk(mMtx);
        mRunning = false;
        mCv.notifyAll();
        more = !mClosed && !mItems.empty();
        if (!more) mScheduled = false;
    }
    // Always succeeds: this thread is in the pool
    if (more) CallbackPool::instance().schedule(shared_from_this());
}

void CallbackQueue::waitForCallback() {
    if (tCurrent == this) return;
    UniqueLock<Mutex> lk(mMtx);
    uint64_t seq = mRunSeq;
    while (mRunning && mRunSeq == seq) mCv.wait(lk);
}

void CallbackQueue::close() {
    std::deque<Item> dropped;
    std::function<void(uint64_t)> reporter;
    {
        UniqueLock<Mutex> lk(mMtx);
        mClosed = true;
        dropped.swap(mItems);
        mBytes = 0;
        mFrames = 0;
        reporter.swap(mDropReporter);
        if (tCurrent != this) {
            while (mRunning) mCv.wait(lk);
        }
    }
}

}  // namespace detail

namespace utils {

void setCallbackThreadCount(unsigned count) {
    detail::CallbackPool::instance().setThreadCount(count);
}

}  // namespace utils
}  // namespace itscam
