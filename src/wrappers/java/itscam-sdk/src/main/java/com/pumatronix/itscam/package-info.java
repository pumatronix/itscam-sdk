/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
/**
 * Idiomatic Java wrapper for the Pumatronix ITSCAM Client SDK.
 *
 * <p>Three coexisting client surfaces back the same C++ core through the
 * SDK's stable C ABI:
 *
 * <ul>
 *   <li>{@link com.pumatronix.itscam.ItscamClient} &mdash; binary TCP on
 *       port 60000 for real-time triggers, GPIO/serial I/O,
 *       exposure-group accumulation and very low-latency capture.</li>
 *   <li>{@link com.pumatronix.itscam.ItscamRestClient} &mdash; REST/JSON
 *       over HTTP/HTTPS for equipment configuration (profiles, OCR,
 *       classifier, lanes, ITSCAM PRO server hooks, ...).
 *       <strong>Always</strong> requires a {@code login()} call first.</li>
 *   <li>{@link com.pumatronix.itscam.ItscamCgiClient} &mdash; legacy CGI
 *       endpoints ({@code snapshot.cgi}, {@code lastframe.cgi},
 *       {@code mjpegvideo.cgi}, {@code reboot.cgi}). Authentication is
 *       <em>opt-in</em>; the camera defaults to anonymous CGI access
 *       ({@code configCgi.blockAPI = false}).</li>
 * </ul>
 *
 * <p>All three classes are {@link java.lang.AutoCloseable} and should be
 * used with try-with-resources to release the underlying native handle.
 *
 * <p>Native library resolution order:
 *
 * <ol>
 *   <li>System property {@code -Ditscam.sdk.library=/path/to/libitscam_sdk.so}.</li>
 *   <li>{@code java.library.path} / {@code LD_LIBRARY_PATH} via JNA.</li>
 *   <li>Library bundled in the JAR at
 *       {@code META-INF/native/<os>-<arch>/libitscam_sdk.so}.</li>
 * </ol>
 */
package com.pumatronix.itscam;
