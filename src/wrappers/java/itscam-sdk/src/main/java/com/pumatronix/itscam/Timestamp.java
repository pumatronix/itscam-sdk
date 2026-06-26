/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.SimpleTimeZone;
import java.util.TimeZone;

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

    /** Convert to a {@link Calendar} carrying the original tz offset. */
    public Calendar toCalendar() {
        TimeZone zone = new SimpleTimeZone(timezoneOffsetSeconds * 1000,
                offsetTimeZoneId(timezoneOffsetSeconds));
        Calendar calendar = new GregorianCalendar(zone);
        calendar.clear();
        calendar.set(Math.max(1, year), Math.max(1, month) - 1,
                Math.max(1, day), hour, minute, second);
        calendar.set(Calendar.MILLISECOND, millisecond);
        return calendar;
    }

    /** Convert to an instant represented by {@link Date}. */
    public Date toDate() {
        return toCalendar().getTime();
    }

    /** ISO 8601 string with millisecond precision. */
    public String toIso8601() {
        return String.format("%04d-%02d-%02dT%02d:%02d:%02d.%03d",
                year, month, day, hour, minute, second, millisecond);
    }

    @Override
    public String toString() { return toIso8601(); }

    private static String offsetTimeZoneId(int offsetSeconds) {
        int absoluteSeconds = Math.abs(offsetSeconds);
        int hours = absoluteSeconds / 3600;
        int minutes = (absoluteSeconds % 3600) / 60;
        return String.format("GMT%s%02d:%02d",
                offsetSeconds < 0 ? "-" : "+", hours, minutes);
    }
}
