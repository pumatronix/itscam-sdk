/*
 *  smoke.c -- C API consumer smoke test, against tests/standin/server.js.
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Plain C99: the C ABI is what every wrapper binds, so this one runs with
 *  no C++ in the consumer at all.  Same checks as the other languages
 *  (tests/README.md), on the REST and binary clients.
 *
 *  Environment: ITSCAM_TEST_HOST (127.0.0.1), ITSCAM_TEST_PORT (18080),
 *  ITSCAM_TEST_TLS_PORT (18443), ITSCAM_TEST_CA (tests/standin/tls/cert.pem).
 */

#define _POSIX_C_SOURCE 199309L  /* nanosleep */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#include "c_api/itscam_sdk_c.h"
#include "c_api/itscam_rest_client_c.h"

static int failures = 0;

static void check(int ok, const char* name, const char* detail) {
    printf("%s %s%s%s\n", ok ? "PASS" : "FAIL", name,
           detail && *detail ? ": " : "", detail ? detail : "");
    if (!ok) ++failures;
}

static void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

/* Written by the SDK's callback pool and read by main: the compilers' atomic
 * builtins (GCC, MinGW, Clang), as C99 has no atomics of its own */
static int telemetry_received = 0;
static int received_count(void) { return __sync_fetch_and_add(&telemetry_received, 0); }
static void on_telemetry(const ITSCAM_TelemetryUpdate* update, void* user_data) {
    (void)user_data;
    if (update && update->json && strstr(update->json, "\"version\":1")) {
        __sync_fetch_and_add(&telemetry_received, 1);
    }
}

static const char* env(const char* name, const char* fallback) {
    const char* v = getenv(name);
    return v && *v ? v : fallback;
}

int main(void) {
    const char* host = env("ITSCAM_TEST_HOST", "127.0.0.1");
    unsigned short port = (unsigned short)atoi(env("ITSCAM_TEST_PORT", "18080"));
    ITSCAM_RestClient* rest;
    ITSCAM_String* out = NULL;
    ITSCAM_ErrorCode rc;
    ITSCAM_Client* client;

    printf("C smoke against %s:%u\n", host, port);

    /* ITSCAM_TEST_EXPECT_VERSION: the library under test, so that a stale
     * copy found first on the search path fails instead of passing */
    check(ITSCAM_getVersion() && *ITSCAM_getVersion() &&
              (!*env("ITSCAM_TEST_EXPECT_VERSION", "") ||
               strcmp(ITSCAM_getVersion(), env("ITSCAM_TEST_EXPECT_VERSION", "")) == 0),
          "version", ITSCAM_getVersion());

    rest = ITSCAM_RestClient_create();
    check(rest != NULL, "rest create", "");
    if (rest) {
        ITSCAM_RestClient_setBaseUrl(rest, host, port, "http");
        rc = ITSCAM_RestClient_login(rest, "admin", "1234", 10000, NULL);
        check(rc == ITSCAM_OK, "rest login", ITSCAM_getLastError());

        rc = ITSCAM_RestClient_getProfiles(rest, 10000, &out);
        if (rc != ITSCAM_OK || !out) {
            check(0, "rest typed get", ITSCAM_getLastError());
        } else {
            const char* json = ITSCAM_String_data(out);
            check(strstr(json, "\"id\":3") != NULL &&
                      strstr(json, "C\xC3\xA2mera S\xC3\xA3o Jo\xC3\xA3o") != NULL,
                  "rest typed get", json);
            ITSCAM_String_destroy(out);
        }

        out = NULL;
        rc = ITSCAM_RestClient_httpGet(rest, "/api/does-not-exist", 10000, &out);
        check(rc == ITSCAM_ERROR_INVALID_PARAMETER,
              "rest 404 maps to ITSCAM_ERROR_INVALID_PARAMETER", ITSCAM_getLastError());
        if (out) ITSCAM_String_destroy(out);

        out = NULL;
        rc = ITSCAM_RestClient_getSystemTelemetry(rest, 10000, &out);
        if (rc != ITSCAM_OK || !out) {
            check(0, "telemetry read", ITSCAM_getLastError());
        } else {
            const char* json = ITSCAM_String_data(out);
            check(strstr(json, "\"status\":\"warning\"") != NULL &&
                      strstr(json, "\"id\":\"storageFull\"") != NULL,
                  "telemetry read", "");
            ITSCAM_String_destroy(out);
        }

        {
            /* The stand-in pushes every 200 ms */
            uint64_t sequence = 0;
            int64_t received_at = 0;
            int i;
            rc = ITSCAM_RestClient_startTelemetryStream(rest, on_telemetry, NULL, 0);
            for (i = 0; i < 100 && received_count() < 2; ++i) sleep_ms(50);
            ITSCAM_RestClient_stopTelemetryStream(rest);
            out = NULL;
            ITSCAM_RestClient_latestTelemetry(rest, &sequence, &received_at, &out);
            if (out) ITSCAM_String_destroy(out);
            check(rc == ITSCAM_OK && received_count() >= 2 && sequence >= 2,
                  "telemetry stream", rc == ITSCAM_OK ? "" : ITSCAM_getLastError());
        }
        ITSCAM_RestClient_destroy(rest);
    }

    /* The stand-in's certificate is self-signed: nobody trusts it */
    {
        unsigned short tls_port = (unsigned short)atoi(env("ITSCAM_TEST_TLS_PORT", "18443"));
        ITSCAM_RestClient* https = ITSCAM_RestClient_create();
        ITSCAM_RestClient_setBaseUrl(https, host, tls_port, "https");
        rc = ITSCAM_RestClient_login(https, "admin", "1234", 10000, NULL);
        check(rc != ITSCAM_OK, "https refuses an untrusted certificate",
              rc != ITSCAM_OK ? ITSCAM_getLastError() : "accepted");
        ITSCAM_RestClient_destroy(https);

        https = ITSCAM_RestClient_create();
        ITSCAM_RestClient_setBaseUrl(https, host, tls_port, "https");
        ITSCAM_RestClient_setCaCertFile(https, env("ITSCAM_TEST_CA", "tests/standin/tls/cert.pem"));
        rc = ITSCAM_RestClient_login(https, "admin", "1234", 10000, NULL);
        check(rc == ITSCAM_OK, "https with the certificate trusted",
              rc == ITSCAM_OK ? "" : ITSCAM_getLastError());
        ITSCAM_RestClient_destroy(https);
    }

    /* Nothing listens on port 1: fails fast and leaves no thread behind */
    client = ITSCAM_Client_create();
    check(client != NULL, "binary create", "");
    if (client) {
        rc = ITSCAM_Client_connect(client, host, 1, 2000, NULL);
        check(rc != ITSCAM_OK, "binary connect to a closed port fails",
              ITSCAM_getLastError());
        ITSCAM_Client_destroy(client);
    }

    printf("%s\n", failures ? "C smoke: FAILED" : "C smoke: OK");
    return failures ? 1 : 0;
}
