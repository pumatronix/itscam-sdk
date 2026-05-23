/*
 *  itscam_mbedtls_config.h
 *
 *  ITSCAM Client SDK - mbedTLS user configuration overlay
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  This header is included by every mbedTLS translation unit via
 *  -DMBEDTLS_USER_CONFIG_FILE="itscam_mbedtls_config.h".  It runs AFTER
 *  the upstream default config and lets us trim mbedTLS down to a
 *  client-only TLS profile.
 *
 *  Goals:
 *    - TLS 1.2 + TLS 1.3 client support over TCP
 *    - X.509 certificate verification (PEM CA bundles)
 *    - AEAD cipher suites required by modern ITSCAM cameras
 *    - No server-side support (no listen sockets, no PSK server)
 *    - No filesystem dependency by default (use mbedtls_x509_crt_parse_*
 *      from in-memory buffers)
 */
#ifndef ITSCAM_MBEDTLS_CONFIG_H
#define ITSCAM_MBEDTLS_CONFIG_H

/* -------------------------------------------------------------------------
 *  Disable server-only pieces we never use in a client SDK
 * ------------------------------------------------------------------------- */

#undef MBEDTLS_SSL_SRV_C
#undef MBEDTLS_SSL_COOKIE_C
#undef MBEDTLS_SSL_TICKET_C
#undef MBEDTLS_SSL_CACHE_C

/* -------------------------------------------------------------------------
 *  Enable the client features httplib's SSL backend relies on
 * ------------------------------------------------------------------------- */

#ifndef MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_CLI_C
#endif

#ifndef MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_TLS_C
#endif

#ifndef MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_PROTO_TLS1_2
#endif

#ifndef MBEDTLS_SSL_PROTO_TLS1_3
#define MBEDTLS_SSL_PROTO_TLS1_3
#endif

/* TLS 1.3 in mbedTLS 3.6 requires the PSA crypto subsystem. */
#ifndef MBEDTLS_USE_PSA_CRYPTO
#define MBEDTLS_USE_PSA_CRYPTO
#endif

#ifndef MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_PSA_CRYPTO_C
#endif

/* X.509 verification (no parsing from filesystem unless caller wants it) */
#ifndef MBEDTLS_X509_USE_C
#define MBEDTLS_X509_USE_C
#endif
#ifndef MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_CRT_PARSE_C
#endif
#ifndef MBEDTLS_X509_CRL_PARSE_C
#define MBEDTLS_X509_CRL_PARSE_C
#endif

/* Enable filesystem helpers so callers can load PEM bundles by path. */
#ifndef MBEDTLS_FS_IO
#define MBEDTLS_FS_IO
#endif

/* PEM + base64 input formats. */
#ifndef MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PEM_PARSE_C
#endif
#ifndef MBEDTLS_BASE64_C
#define MBEDTLS_BASE64_C
#endif

/* -------------------------------------------------------------------------
 *  Cipher suites required by modern ITSCAM webapp / nginx fronts
 * ------------------------------------------------------------------------- */

#ifndef MBEDTLS_GCM_C
#define MBEDTLS_GCM_C
#endif
#ifndef MBEDTLS_CHACHAPOLY_C
#define MBEDTLS_CHACHAPOLY_C
#endif
#ifndef MBEDTLS_CHACHA20_C
#define MBEDTLS_CHACHA20_C
#endif
#ifndef MBEDTLS_POLY1305_C
#define MBEDTLS_POLY1305_C
#endif
#ifndef MBEDTLS_AES_C
#define MBEDTLS_AES_C
#endif

/* Key exchange */
#ifndef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#endif
#ifndef MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#endif
#ifndef MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#endif

/* Hashes */
#ifndef MBEDTLS_SHA256_C
#define MBEDTLS_SHA256_C
#endif
#ifndef MBEDTLS_SHA384_C
#define MBEDTLS_SHA384_C
#endif
#ifndef MBEDTLS_SHA512_C
#define MBEDTLS_SHA512_C
#endif

/* Public-key */
#ifndef MBEDTLS_RSA_C
#define MBEDTLS_RSA_C
#endif
#ifndef MBEDTLS_ECDSA_C
#define MBEDTLS_ECDSA_C
#endif
#ifndef MBEDTLS_ECDH_C
#define MBEDTLS_ECDH_C
#endif
#ifndef MBEDTLS_ECP_C
#define MBEDTLS_ECP_C
#endif

/* RNG: prefer entropy + DRBG based on hashes only (no /dev/random hard
 * dependency on Windows). */
#ifndef MBEDTLS_ENTROPY_C
#define MBEDTLS_ENTROPY_C
#endif
#ifndef MBEDTLS_CTR_DRBG_C
#define MBEDTLS_CTR_DRBG_C
#endif

/* -------------------------------------------------------------------------
 *  Disable rarely-used features to keep the static archive small
 * ------------------------------------------------------------------------- */

#undef MBEDTLS_DES_C
#undef MBEDTLS_BLOWFISH_C
#undef MBEDTLS_CAMELLIA_C
#undef MBEDTLS_ARIA_C
#undef MBEDTLS_NET_C  /* we feed the socket FD directly to mbedtls_ssl_set_bio */

/* MD5 is required by cpp-httplib's digest-auth helper -- keep it on. */
#ifndef MBEDTLS_MD5_C
#define MBEDTLS_MD5_C
#endif

#endif /* ITSCAM_MBEDTLS_CONFIG_H */
