/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.internal;

import com.sun.jna.Pointer;

/**
 * A client's native handle, shared by its calls.  {@link #close} may run
 * while another thread is still in a native call: the handle is then
 * destroyed once the last such call returns.
 *
 * <p>Every native call takes the handle with {@link #acquire} or
 * {@link #tryAcquire} and gives it back with {@link #release}.
 */
public final class NativeHandle {

    /** Frees the native handle. */
    public interface Destroyer {
        void destroy(Pointer handle);
    }

    /** Runs with the handle while the client is closing. */
    public interface Closer {
        void beforeDestroy(Pointer handle);
    }

    private final String closedMessage;
    private final Destroyer destroyer;

    // Guarded by this
    private Pointer handle;
    private int users;
    private volatile boolean closed;

    public NativeHandle(Pointer handle, String closedMessage,
                        Destroyer destroyer) {
        this.handle = handle;
        this.closedMessage = closedMessage;
        this.destroyer = destroyer;
    }

    /**
     * Returns the handle, held until {@link #release}.
     *
     * @throws IllegalStateException once the client is closed
     */
    public synchronized Pointer acquire() {
        if (closed) throw new IllegalStateException(closedMessage);
        users++;
        return handle;
    }

    /** Like {@link #acquire}, but returns null once the client is closed. */
    public synchronized Pointer tryAcquire() {
        if (closed) return null;
        users++;
        return handle;
    }

    /**
     * Gives back a handle from {@link #acquire} or {@link #tryAcquire}.  The
     * last call to return after {@link #close} destroys the handle, on a
     * thread of its own: a callback may be waiting for that call, or for an
     * async call queued on {@link Async}, and destroying waits for the
     * callbacks.
     */
    public void release() {
        final Pointer last = drop();
        if (last != null) {
            Thread t = new Thread(new Runnable() {
                @Override
                public void run() {
                    destroyer.destroy(last);
                }
            }, "itscam-sdk-java-destroy");
            t.setDaemon(true);
            t.start();
        }
    }

    public boolean isClosed() {
        return closed;
    }

    /**
     * Marks the client closed and runs {@code beforeDestroy} with the
     * handle, once.  The handle is destroyed here if no call holds it,
     * otherwise when the last one returns.
     */
    public void close(Closer beforeDestroy) {
        Pointer h;
        synchronized (this) {
            if (closed) return;
            closed = true;
            users++;
            h = handle;
        }
        try {
            if (beforeDestroy != null) beforeDestroy.beforeDestroy(h);
        } finally {
            Pointer last = drop();
            if (last != null) destroyer.destroy(last);
        }
    }

    private synchronized Pointer drop() {
        if (--users > 0 || !closed) return null;
        Pointer h = handle;
        handle = null;
        return h;
    }
}
