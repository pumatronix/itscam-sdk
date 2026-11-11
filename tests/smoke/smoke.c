/*
 *  smoke.c -- C API consumer smoke test, against tests/standin/server.js.
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Plain C99: the C ABI is what every wrapper binds, so this one runs with
 *  no C++ in the consumer at all.  Same checks as the other languages
 *  (tests/README.md), on the REST and binary clients.
 *
 *  Environment: ITSCAM_TEST_HOST (127.0.0.1), ITSCAM_TEST_PORT (18080).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_api/itscam_sdk_c.h"
#include "c_api/itscam_rest_client_c.h"

static int failures = 0;

static void check(int ok, const char* name, const char* detail) {
    printf("%s %s%s%s\n", ok ? "PASS" : "FAIL", name,
           detail && *detail ? ": " : "", detail ? detail : "");
    if (!ok) ++failures;
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

    check(ITSCAM_getVersion() && *ITSCAM_getVersion(), "version", ITSCAM_getVersion());

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
        ITSCAM_RestClient_destroy(rest);
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
