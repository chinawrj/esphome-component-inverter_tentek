/**
 * @file md5_wrapper.h
 * @brief MD5 calculation wrapper for C code
 * 
 * This wrapper provides a C-compatible interface to ESPHome's MD5 implementation,
 * allowing C code (set_power_service.c) to calculate MD5 hashes without directly
 * depending on mbedtls library.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate MD5 hash of input data
 * 
 * This function provides a simple C interface to calculate MD5 hashes,
 * compatible with the original mbedtls MD5 implementation.
 * 
 * @param input Pointer to input data
 * @param ilen Length of input data in bytes
 * @param output Pointer to output buffer (must be at least 16 bytes)
 * @return 0 on success, non-zero on failure
 * 
 * @note Output buffer format: 16 bytes of raw MD5 hash
 * 
 * Example usage:
 * @code
 * uint8_t hash[16];
 * const char *data = "Hello World";
 * md5_calculate((const uint8_t*)data, strlen(data), hash);
 * @endcode
 */
int md5_calculate(const uint8_t *input, size_t ilen, uint8_t output[16]);

#ifdef __cplusplus
}
#endif
