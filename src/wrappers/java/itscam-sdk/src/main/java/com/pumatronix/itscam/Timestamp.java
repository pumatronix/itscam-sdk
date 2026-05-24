/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import java.time.LocalDateTime;
import java.time.OffsetDateTime;
import java.time.ZoneOffset;

/**
 * Timestamp returned by the camera, broken down into calendar fields
 * plus a timezone offset (seconds from UTC).  Mirrors
 * {@code ITSCAM_Timestamp}.
 */
public final class Timestamp {

    private final int year;
    private final int month;
    private final int day;
    private final int hour;
    private final int minute;
    private final int second;
    private final int millisecond;
    private final int timezoneOffsetSeconds;

    public Timestamp(int year, int month, int day,
                     int hour, int minute, int second,
                     int millisecond, int timezoneOffsetSeconds) {
        this.year = year;
        this.month = month;
        this.day = day;
        this.hour = hour;
        this.minute = minute;
        this.second = second;
        this.millisecond = millisecond;
        this.timezoneOffsetSeconds = timezoneOffsetSeconds;
    }

    public int year() { return year; }
    public int month() { return month; }
    public int day() { return day; }
    public int hour() { return hour; }
    public int minute() { return minute; }
    public int second() { return second; }
    public int millisecond() { return millisecond; }
    public int timezoneOffsetSeconds() { return timezoneOffsetSeconds; }

    /** Convert to a naive {@link LocalDateTime} (timezone discarded). */
    public LocalDateTime toLocalDateTime() {
        return LocalDateTime.of(year, Math.max(1, month), Math.max(1, day),
                hour, minute, second, millisecond * 1_000_000);
    }

    /** Convert to {@link OffsetDateTime} carrying the original tz offset. */
    public OffsetDateTime toOffsetDateTime() {
        return OffsetDateTime.of(toLocalDateTime(),
                ZoneOffset.ofTotalSeconds(timezoneOffsetSeconds));
    }

    /** ISO 8601 string with millisecond precision. */
    public String toIso8601() {
        return String.format("%04d-%02d-%02dT%02d:%02d:%02d.%03d",
                year, month, day, hour, minute, second, millisecond);
    }

    @Override
    public String toString() { return toIso8601(); }
}
