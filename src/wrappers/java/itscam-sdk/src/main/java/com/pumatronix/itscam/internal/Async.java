/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.internal;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

/** Shared background executor for the Java wrapper's JDK 7 async surface. */
public final class Async {
    private static final ExecutorService EXECUTOR = Executors.newCachedThreadPool(
            new ThreadFactory() {
                private final AtomicInteger nextId = new AtomicInteger(1);

                @Override
                public Thread newThread(Runnable r) {
                    Thread t = new Thread(r,
                            "itscam-sdk-java-" + nextId.getAndIncrement());
                    t.setDaemon(true);
                    return t;
                }
            });

    private Async() {}

    public static Future<Void> run(final Runnable task) {
        return EXECUTOR.submit(new Callable<Void>() {
            @Override
            public Void call() {
                task.run();
                return null;
            }
        });
    }

    public static <T> Future<T> submit(Callable<T> task) {
        return EXECUTOR.submit(task);
    }
}