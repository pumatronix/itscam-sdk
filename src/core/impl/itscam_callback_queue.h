/*
 *  itscam_callback_queue.h
 *
 *  ITSCAM Client SDK - Callback delivery (internal)
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  The SDK's own threads (socket, handler, MJPEG stream, ...) never run user
 *  callbacks: they post them to their client's CallbackQueue and carry on.
 *  A small pool of threads shared by every client runs the queued callbacks,
 *  in order and one at a time per client, with no SDK lock held.  A callback
 *  that blocks, or that a binding relays to another thread (Node.js runs
 *  them on its event loop), then only delays the later callbacks of its
 *  client, never the protocol.
 *
 *  This is an INTERNAL header.  It is not installed and not part of the
 *  public SDK surface.
 */
#pragma once

#include "../itscam_os.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>

namespace itscam {
namespace detail {

class CallbackQueue : public std::enable_shared_from_this<CallbackQueue> {
public:
    /// What a full queue does with a new callback
    enum class Kind {
        Event,      ///< drops it
        Essential,  ///< queues it anyway (connection state changes)
        Frame,      ///< drops its oldest queued frame instead
    };

    /// Limits of one client's pending callbacks
    static constexpr size_t kMaxPending      = 256;
    static constexpr size_t kMaxPendingBytes = 32u << 20;
    static constexpr size_t kMaxFrames       = 4;

    static std::shared_ptr<CallbackQueue> create();

    CallbackQueue(const CallbackQueue&) = delete;
    CallbackQueue& operator=(const CallbackQueue&) = delete;

    /// Queues fn, whose payload takes @p bytes.  Returns false if it was
    /// dropped: the queue is closed or full (see Kind).
    bool post(std::function<void()> fn, size_t bytes = 0,
              Kind kind = Kind::Event);

    /// Once the queue has drained after dropping callbacks, it calls
    /// @p reporter like a callback, with how many it dropped.
    void setDropReporter(std::function<void(uint64_t)> reporter);

    /// Waits for the callback in progress, if any, unless called from it.
    void waitForCallback();

    /// Drops the pending callbacks and admits no more, then waits for the
    /// one in progress, unless called from it.
    void close();

    /// Runs the next callback (pool threads only).
    void runNext();

private:
    CallbackQueue() = default;

    struct Item {
        std::function<void()> fn;
        size_t bytes = 0;
        Kind kind = Kind::Event;
    };

    os::Mutex                  mMtx;
    os::ConditionVariable      mCv;       // signalled when a callback returns
    std::deque<Item>           mItems;
    size_t                     mBytes = 0;
    size_t                     mFrames = 0;
    uint64_t                   mDropped = 0;
    uint64_t                   mRunSeq = 0;  // callbacks started
    bool                       mScheduled = false;  // waiting for or on a pool thread
    bool                       mRunning = false;
    bool                       mClosed = false;
    std::function<void(uint64_t)> mDropReporter;

    // Queue whose callback the calling thread runs
    static thread_local const CallbackQueue* tCurrent;
};

}  // namespace detail
}  // namespace itscam
